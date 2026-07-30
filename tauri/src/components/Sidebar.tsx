import { useMemo, useState } from "react";
import type { VaultEntry } from "../lib/types";

interface Props {
  entries: VaultEntry[];
  active: string | null;
  onOpen: (path: string) => void;
  onRename: (path: string, newName: string) => void;
  onDelete: (path: string) => void;
  onCreate: (dir?: string) => void;
  style?: React.CSSProperties;
}

function TreeItem({
  entry,
  depth,
  active,
  onOpen,
  onRename,
  onDelete,
  onCreate,
}: {
  entry: VaultEntry;
  depth: number;
  active: string | null;
  onOpen: (path: string) => void;
  onRename: (path: string, newName: string) => void;
  onDelete: (path: string) => void;
  onCreate: (dir?: string) => void;
}) {
  const [expanded, setExpanded] = useState(true);
  const isActive = !entry.is_dir && active === entry.path;

  const handleClick = () => {
    if (entry.is_dir) {
      setExpanded((e) => !e);
    } else {
      onOpen(entry.path);
    }
  };

  const handleContextMenu = (e: React.MouseEvent) => {
    e.preventDefault();
    if (entry.is_dir) {
      const action = window.prompt(`Folder: ${entry.name}\nType 'create' to add a note`, "");
      if (action?.toLowerCase() === "create") onCreate(entry.path);
      return;
    }
    const action = window.prompt(`Actions for ${entry.name}\nType 'rename' or 'delete'`, "");
    if (action === "rename") {
      const newName = window.prompt("New name", entry.name);
      if (newName) onRename(entry.path, newName);
    } else if (action === "delete") {
      onDelete(entry.path);
    }
  };

  return (
    <div key={entry.path}>
      <div
        className={`tree-item ${isActive ? "active" : ""}`}
        style={{ paddingLeft: `${0.6 + depth * 0.75}rem` }}
        onClick={handleClick}
        onContextMenu={handleContextMenu}
        title={entry.path}
      >
        <span className="tree-icon">{entry.is_dir ? (expanded ? "▾" : "▸") : "📝"}</span>
        <span className="tree-name">{entry.name}</span>
      </div>
      {entry.is_dir && entry.children.length > 0 && expanded && (
        <div>{renderTree(entry.children, depth + 1, active, onOpen, onRename, onDelete, onCreate)}</div>
      )}
    </div>
  );
}

function renderTree(
  items: VaultEntry[],
  depth: number,
  active: string | null,
  onOpen: (path: string) => void,
  onRename: (path: string, newName: string) => void,
  onDelete: (path: string) => void,
  onCreate: (dir?: string) => void
) {
  return items.map((entry) => (
    <TreeItem
      key={entry.path}
      entry={entry}
      depth={depth}
      active={active}
      onOpen={onOpen}
      onRename={onRename}
      onDelete={onDelete}
      onCreate={onCreate}
    />
  ));
}

function filterEntries(entries: VaultEntry[], query: string): VaultEntry[] {
  const q = query.toLowerCase();
  return entries
    .map((entry) => {
      const children = entry.is_dir ? filterEntries(entry.children, query) : [];
      if (entry.name.toLowerCase().includes(q) || children.length > 0) {
        return { ...entry, children };
      }
      return null;
    })
    .filter(Boolean) as VaultEntry[];
}

export default function Sidebar({
  entries,
  active,
  onOpen,
  onRename,
  onDelete,
  onCreate,
  style,
}: Props) {
  const [query, setQuery] = useState("");
  const visible = useMemo(() => (query ? filterEntries(entries, query) : entries), [entries, query]);

  return (
    <aside className="sidebar" style={style}>
      <div className="sidebar-header">
        <input
          className="sidebar-search"
          placeholder="Search notes..."
          value={query}
          onChange={(e) => setQuery(e.target.value)}
        />
      </div>
      <div className="sidebar-tree">
        {renderTree(visible, 0, active, onOpen, onRename, onDelete, onCreate)}
        {visible.length === 0 && <div className="sidebar-empty">No notes found</div>}
      </div>
    </aside>
  );
}
