import { invoke } from "@tauri-apps/api/core";
import * as React from "react";
import { useEffect, useRef, useState } from "react";
import type {
  Graph,
  LoadedPlugin,
  PluginApi,
  PluginCommand,
  PluginPanel,
  PluginInstance,
} from "./types";

export class PluginManager {
  private plugins: LoadedPlugin[] = [];
  private listeners: Map<string, Set<(...args: unknown[]) => void>> = new Map();
  private commands: PluginCommand[] = [];
  private panels: PluginPanel[] = [];
  private api: PluginApi | null = null;
  private onChange: () => void;
  private currentVault: string | null = null;

  constructor(onChange: () => void) {
    this.onChange = onChange;
  }

  setVault(vault: string | null, openNote: (path: string) => Promise<void>) {
    this.api = this.buildApi(vault, openNote);
    if (this.currentVault !== vault) {
      this.currentVault = vault;
      this.load();
    }
  }

  async load() {
    this.plugins = [];
    this.commands = [];
    this.panels = [];
    try {
      const loaded = (await invoke("load_plugins")) as LoadedPlugin[];
      this.plugins = loaded.map((p) => ({ ...p, enabled: true }));
      for (const plugin of this.plugins) {
        if (!plugin.enabled) continue;
        try {
          const factory = new Function(
            "api",
            "React",
            `"use strict";\n${plugin.code}\nreturn typeof plugin !== "undefined" ? plugin : undefined;`
          );
          const instance = factory(this.api, React) as PluginInstance | undefined;
          if (instance && typeof instance === "object") {
            if (typeof instance.onLoad === "function") {
              instance.onLoad(this.api!);
            } else if (typeof instance.default === "function") {
              (instance.default as (api: PluginApi) => void)(this.api!);
            }
          }
        } catch (err) {
          console.error(`[plugin] failed to load ${plugin.name}:`, err);
        }
      }
    } catch (err) {
      console.error("[plugin] load_plugins failed:", err);
    }
    this.onChange();
  }

  getApi(): PluginApi | null {
    return this.api;
  }

  getCommands(): PluginCommand[] {
    return this.commands;
  }

  getPanels(): PluginPanel[] {
    return this.panels;
  }

  getPlugins(): LoadedPlugin[] {
    return this.plugins;
  }

  emit(event: string, ...args: unknown[]) {
    const set = this.listeners.get(event);
    if (set) {
      set.forEach((handler) => {
        try {
          handler(...args);
        } catch (err) {
          console.error(`[plugin] event ${event} handler error:`, err);
        }
      });
    }
  }

  private buildApi(vault: string | null, openNote: (path: string) => Promise<void>): PluginApi {
    const api: PluginApi = {
      React,
      vault: {
        path: vault,
        getGraph: async () => {
          if (!vault) return { nodes: [], edges: [] };
          return (await invoke("get_vault_graph", { vaultPath: vault })) as Graph;
        },
        search: async (query: string) => {
          if (!vault) return [];
          return (await invoke("search_vault", { vaultPath: vault, query })) as string[];
        },
        createNote: async (title: string) => {
          if (!vault) throw new Error("No vault open");
          return (await invoke("create_wiki_note", { vaultPath: vault, title })) as string;
        },
        openNote: async (path: string) => openNote(path),
        getBacklinks: async (notePath: string) => {
          if (!vault) return [];
          return (await invoke("get_backlinks", { vaultPath: vault, notePath })) as string[];
        },
      },
      on: (event: string, handler: (...args: unknown[]) => void) => {
        if (!this.listeners.has(event)) this.listeners.set(event, new Set());
        this.listeners.get(event)!.add(handler);
      },
      off: (event: string, handler: (...args: unknown[]) => void) => {
        this.listeners.get(event)?.delete(handler);
      },
      registerCommand: (cmd: PluginCommand) => {
        this.commands.push(cmd);
        this.onChange();
      },
      registerPanel: (panel: PluginPanel) => {
        this.panels.push(panel);
        this.onChange();
      },
      log: (...args: unknown[]) => console.log(`[plugin]`, ...args),
    };
    return api;
  }
}

export function usePluginManager(vault: string | null, openNote: (path: string) => Promise<void>) {
  const [, setVersion] = useState(0);
  const openNoteRef = useRef(openNote);
  openNoteRef.current = openNote;

  const managerRef = useRef<PluginManager | null>(null);
  if (!managerRef.current) {
    managerRef.current = new PluginManager(() => setVersion((v) => v + 1));
  }
  const manager = managerRef.current;

  useEffect(() => {
    if (vault) {
      // Always pass a fresh wrapper so the API never holds a stale openNote,
      // but only reload plugins when the vault itself changes.
      manager.setVault(vault, (path) => openNoteRef.current(path));
    }
  }, [manager, vault]);

  return manager;
}
