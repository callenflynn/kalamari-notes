import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import { open } from "@tauri-apps/plugin-dialog";
import { loadConfig, saveConfig } from "./lib/config";
import { usePluginManager } from "./lib/plugins";
import type { VaultEntry, Tab, Graph } from "./lib/types";
import Sidebar from "./components/Sidebar";
import TopBar from "./components/TopBar";
import TabBar from "./components/TabBar";
import Editor from "./components/Editor";
import EmptyState from "./components/EmptyState";
import GraphView from "./components/GraphView";
import Backlinks from "./components/Backlinks";
import MetadataPanel from "./components/MetadataPanel";
import SearchPalette from "./components/SearchPalette";
import Resizer from "./components/Resizer";
import "./styles/theme.css";

type View = "editor" | "graph";

function App() {
  const [vault, setVault] = useState<string | null>(null);
  const [entries, setEntries] = useState<VaultEntry[]>([]);
  const [tabs, setTabs] = useState<Tab[]>([]);
  const [activeTab, setActiveTab] = useState<string | null>(null);
  const [dark, setDark] = useState(true);
  const [loaded, setLoaded] = useState(false);
  const [view, setView] = useState<View>("editor");
  const [searchOpen, setSearchOpen] = useState(false);
  const [sidebarWidth, setSidebarWidth] = useState(260);
  const [graph, setGraph] = useState<Graph | null>(null);  // ---------------------------------------------------------------------------
  // Core helpers
  // ---------------------------------------------------------------------------
  const refreshEntries = useCallback(async (path: string) => {
    const list = await invoke<VaultEntry[]>("list_vault", { vaultPath: path });
    setEntries(list);
  }, []);

  const refreshGraph = useCallback(async (path: string) => {
    const g = await invoke<Graph>("get_vault_graph", { vaultPath: path });
    setGraph(g);
  }, []);

  const openNote = useCallback(async (notePath: string) => {
    const v = vaultRef.current;
    if (!v) return;
    if (tabsRef.current.find((t) => t.path === notePath)) {
      setActiveTab(notePath);
      return;
    }
    const content = await invoke<string>("read_note", {
      vaultPath: v,
      notePath,
    });
    const newTab: Tab = { path: notePath, content, savedContent: content, dirty: false };
    setTabs((prev) => [...prev, newTab]);
    setActiveTab(notePath);
  }, []);

  const pluginManager = usePluginManager(vault, openNote);

  // Refs to avoid resetting timers/listeners on every state change
  const tabsRef = useRef(tabs);
  const activeTabRef = useRef(activeTab);
  const vaultRef = useRef(vault);
  const darkRef = useRef(dark);
  const sidebarWidthRef = useRef(sidebarWidth);
  useEffect(() => {
    tabsRef.current = tabs;
  });
  useEffect(() => {
    activeTabRef.current = activeTab;
  });
  useEffect(() => {
    vaultRef.current = vault;
  });
  useEffect(() => {
    darkRef.current = dark;
  }, [dark]);
  useEffect(() => {
    sidebarWidthRef.current = sidebarWidth;
  }, [sidebarWidth]);

  // ---------------------------------------------------------------------------
  // Vault lifecycle
  // ---------------------------------------------------------------------------
  const openVault = useCallback(
    async (path?: string) => {
      let selected = path;
      if (!selected) {
        const dir = await open({ directory: true });
        if (!dir) return;
        selected = dir as string;
      }
      setVault(selected);
      setView("editor");
      await invoke("watch_vault", { vaultPath: selected });
      await refreshEntries(selected);
      await refreshGraph(selected);
      pluginManager.setVault(selected, openNote);
      await saveConfig({
        lastVault: selected,
        darkMode: darkRef.current,
        sidebarWidth: sidebarWidthRef.current,
      });
    },
    [openNote, pluginManager, refreshEntries, refreshGraph]
  );

  useEffect(() => {
    loadConfig().then((cfg) => {
      setDark(cfg.darkMode ?? true);
      if (cfg.sidebarWidth) setSidebarWidth(cfg.sidebarWidth);
      if (cfg.lastVault) {
        openVault(cfg.lastVault).catch(console.error);
      }
      setLoaded(true);
    });
  }, [openVault]);

  useEffect(() => {
    document.documentElement.setAttribute("data-theme", dark ? "dark" : "light");
  }, [dark]);

  // ---------------------------------------------------------------------------
  // File watcher
  // ---------------------------------------------------------------------------
  useEffect(() => {
    const unlisten = listen("vault-changed", () => {
      const v = vaultRef.current;
      if (!v) return;
      refreshEntries(v);
      refreshGraph(v);
      // Refresh active note content if unchanged
      const a = activeTabRef.current;
      setTabs((prev) => {
        const current = prev.find((t) => t.path === a);
        if (!current || current.dirty) return prev;
        // Re-read in background without blocking UI
        invoke<string>("read_note", { vaultPath: v, notePath: current.path })
          .then((content) => {
            setTabs((p) =>
              p.map((x) => (x.path === a ? { ...x, content, savedContent: content, dirty: false } : x))
            );
          })
          .catch(console.error);
        return prev;
      });
    });
    return () => {
      unlisten.then((f) => f());
    };
  }, [vault, refreshEntries, refreshGraph]);

  // ---------------------------------------------------------------------------
  // Auto-save
  // ---------------------------------------------------------------------------
  useEffect(() => {
    const id = setInterval(() => {
      const v = vaultRef.current;
      const a = activeTabRef.current;
      const t = tabsRef.current;
      if (!v || !a) return;
      const tab = t.find((x) => x.path === a);
      if (!tab || !tab.dirty) return;
      const writtenContent = tab.content;
      invoke("write_note", { vaultPath: v, notePath: tab.path, content: writtenContent }).then(() => {
        setTabs((prev) =>
          prev.map((x) =>
            x.path === a
              ? { ...x, savedContent: writtenContent, dirty: x.content !== writtenContent }
              : x
          )
        );
        pluginManager.emit("noteSaved", { path: a, content: writtenContent });
      });
    }, 30000);
    return () => clearInterval(id);
  }, [pluginManager]);

  // ---------------------------------------------------------------------------
  // Note actions
  // ---------------------------------------------------------------------------
  const createNote = useCallback(
    async (dir?: string) => {
      const v = vaultRef.current;
      if (!v) return;
      const name = prompt("Note name");
      if (!name) return;
      const fileName = await invoke<string>("create_note", {
        vaultPath: v,
        relativeDir: dir || null,
        name,
      });
      await refreshEntries(v);
      await openNote(fileName);
      pluginManager.emit("noteCreated", { path: fileName });
    },
    [refreshEntries, openNote, pluginManager]
  );

  const handleWikiLink = useCallback(
    async (target: string) => {
      const v = vaultRef.current;
      if (!v) return;
      const path = await invoke<string>("create_wiki_note", { vaultPath: v, title: target });
      await openNote(path);
      pluginManager.emit("noteCreated", { path });
    },
    [openNote, pluginManager]
  );

  const closeTab = useCallback((path: string) => {
    setTabs((prev) => {
      const next = prev.filter((t) => t.path !== path);
      if (activeTabRef.current === path) {
        setActiveTab(next.length ? next[next.length - 1].path : null);
      }
      return next;
    });
  }, []);

  const saveActive = useCallback(async () => {
    const v = vaultRef.current;
    const a = activeTabRef.current;
    const tab = tabsRef.current.find((t) => t.path === a);
    if (!v || !a || !tab || !tab.dirty) return;
    await invoke("write_note", { vaultPath: v, notePath: tab.path, content: tab.content });
    setTabs((prev) =>
      prev.map((t) => (t.path === a ? { ...t, savedContent: t.content, dirty: false } : t))
    );
    pluginManager.emit("noteSaved", { path: a, content: tab.content });
    refreshGraph(v);
  }, [pluginManager, refreshGraph]);

  const updateActiveContent = useCallback((content: string) => {
    const a = activeTabRef.current;
    setTabs((prev) =>
      prev.map((t) =>
        t.path === a ? { ...t, content, dirty: t.savedContent !== content } : t
      )
    );
  }, []);

  const onRename = useCallback(
    async (oldPath: string, newName: string) => {
      const v = vaultRef.current;
      if (!v) return;
      const newPath = await invoke<string>("rename_note", { vaultPath: v, notePath: oldPath, newName });
      await refreshEntries(v);
      setTabs((prev) =>
        prev.map((t) => (t.path === oldPath ? { ...t, path: newPath } : t))
      );
      if (activeTabRef.current === oldPath) setActiveTab(newPath);
    },
    [refreshEntries]
  );

  const onDelete = useCallback(
    async (path: string) => {
      const v = vaultRef.current;
      if (!v) return;
      if (!confirm(`Delete ${path}?`)) return;
      await invoke("delete_note", { vaultPath: v, notePath: path });
      setTabs((prev) => {
        const next = prev.filter((t) => t.path !== path);
        if (activeTabRef.current === path) {
          setActiveTab(next.length ? next[next.length - 1].path : null);
        }
        return next;
      });
      await refreshEntries(v);
      refreshGraph(v);
    },
    [refreshEntries, refreshGraph]
  );

  const dailyNote = useCallback(async () => {
    const v = vaultRef.current;
    if (!v) return;
    const path = await invoke<string>("get_daily_note", { vaultPath: v });
    await refreshEntries(v);
    await openNote(path);
  }, [refreshEntries, openNote]);

  // ---------------------------------------------------------------------------
  // UI handlers
  // ---------------------------------------------------------------------------
  const toggleTheme = useCallback(() => {
    setDark((d) => !d);
  }, []);

  const handleResize = useCallback((delta: number) => {
    setSidebarWidth((w) => {
      const next = Math.min(Math.max(180, w + delta), 420);
      const v = vaultRef.current;
      if (v) saveConfig({ lastVault: v, darkMode: darkRef.current, sidebarWidth: next });
      return next;
    });
  }, []);

  const openPluginFolder = useCallback(async () => {
    await invoke("open_plugin_folder");
  }, []);

  // ---------------------------------------------------------------------------
  // Keyboard shortcuts
  // ---------------------------------------------------------------------------
  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === "s") {
        e.preventDefault();
        saveActive();
      }
      if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === "n") {
        e.preventDefault();
        createNote();
      }
      if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === "o") {
        e.preventDefault();
        openVault();
      }
      if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === "k") {
        e.preventDefault();
        setSearchOpen(true);
      }
      if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === "g") {
        e.preventDefault();
        setView((v) => (v === "editor" ? "graph" : "editor"));
      }
    };
    window.addEventListener("keydown", handler);
    return () => window.removeEventListener("keydown", handler);
  }, [createNote, openVault, saveActive]);

  // ---------------------------------------------------------------------------
  // Derived state
  // ---------------------------------------------------------------------------
  const activeNote = useMemo(() => {
    return tabs.find((t) => t.path === activeTab) || null;
  }, [activeTab, tabs]);

  const pluginPanels = pluginManager.getPanels();
  const pluginCommands = pluginManager.getCommands();

  if (!loaded) {
    return <div className="app" />;
  }

  return (
    <div className="app">
      <TopBar
        vault={vault}
        dark={dark}
        view={view}
        onOpenVault={() => openVault()}
        onNewNote={() => createNote()}
        onToggleTheme={toggleTheme}
        onToggleView={() => setView((v) => (v === "editor" ? "graph" : "editor"))}
        onDailyNote={dailyNote}
        onSearch={() => setSearchOpen(true)}
        onOpenPluginFolder={openPluginFolder}
      />
      <div className="app-body">
        <Sidebar
          style={{ width: sidebarWidth }}
          entries={entries}
          active={activeTab}
          onOpen={openNote}
          onRename={onRename}
          onDelete={onDelete}
          onCreate={createNote}
        />
        <Resizer onResize={handleResize} />
        <div className="main">
          {tabs.length > 0 ? (
            <>
              <TabBar tabs={tabs} active={activeTab} onSelect={setActiveTab} onClose={closeTab} />
              {view === "graph" ? (
                graph ? (
                  <GraphView graph={graph} dark={dark} onNodeClick={openNote} />
                ) : (
                  <EmptyState title="Graph" subtitle="Open a vault to see the graph" />
                )
              ) : activeNote ? (
                <Editor note={activeNote} onChange={updateActiveContent} onSave={saveActive} onWikiLink={handleWikiLink} />
              ) : (
                <EmptyState title="No note selected" />
              )}
            </>
          ) : (
            <EmptyState title="Kalamari" subtitle="Open a vault or create a new note" hint="Ctrl+N | Ctrl+O | Ctrl+K" />
          )}
        </div>
        <aside className="info-sidebar">
          {activeNote ? (
            <>
              <MetadataPanel note={activeNote} graph={graph} />
              <Backlinks vault={vault} notePath={activeNote.path} onOpen={openNote} />
              {pluginCommands.length > 0 && (
                <div className="info-panel">
                  <h3>Plugin Commands</h3>
                  {pluginCommands.map((cmd) => (
                    <button key={cmd.id} className="plugin-command" onClick={cmd.callback}>
                      {cmd.label}
                    </button>
                  ))}
                </div>
              )}
              {pluginPanels.length > 0 && (
                <div className="info-panel">
                  <h3>Plugins</h3>
                  {pluginPanels.map((panel) => (
                    <div key={panel.id} className="plugin-panel">
                      <h4>{panel.title}</h4>
                      {panel.render()}
                    </div>
                  ))}
                </div>
              )}
            </>
          ) : (
            <div className="info-panel">
              <h3>Second Brain</h3>
              <p className="info-empty">Open a note to see backlinks, metadata, and graph stats.</p>
            </div>
          )}
        </aside>
      </div>
      <SearchPalette vault={vault} isOpen={searchOpen} onClose={() => setSearchOpen(false)} onOpen={openNote} />
    </div>
  );
}

export default App;
