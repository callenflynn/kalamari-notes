import type * as React from "react";

export interface VaultEntry {
  path: string;
  name: string;
  is_dir: boolean;
  children: VaultEntry[];
}

export interface Note {
  path: string;
  content: string;
  dirty: boolean;
}

export interface Tab extends Note {
  savedContent: string;
}

export interface GraphNode {
  id: string;
  path: string;
  title: string;
  exists: boolean;
  in_count: number;
  out_count: number;
}

export interface GraphEdge {
  source: string;
  target: string;
  kind: string;
}

export interface Graph {
  nodes: GraphNode[];
  edges: GraphEdge[];
}

export interface LoadedPlugin {
  name: string;
  version: string;
  manifest: Record<string, unknown>;
  code: string;
  enabled: boolean;
}

export interface PluginCommand {
  id: string;
  label: string;
  callback: () => void;
}

export interface PluginPanel {
  id: string;
  title: string;
  render: () => React.ReactNode;
}

export interface PluginApi {
  React: typeof React;
  vault: {
    path: string | null;
    getGraph: () => Promise<Graph>;
    search: (query: string) => Promise<string[]>;
    createNote: (title: string) => Promise<string>;
    openNote: (path: string) => Promise<void>;
    getBacklinks: (notePath: string) => Promise<string[]>;
  };
  on: (event: string, handler: (...args: unknown[]) => void) => void;
  off: (event: string, handler: (...args: unknown[]) => void) => void;
  registerCommand: (cmd: PluginCommand) => void;
  registerPanel: (panel: PluginPanel) => void;
  log: (...args: unknown[]) => void;
}

export interface PluginInstance {
  onLoad?: (api: PluginApi) => void;
  onUnload?: (api: PluginApi) => void;
  // legacy shape used by dynamic plugins
  default?: (api: PluginApi) => void;
}
