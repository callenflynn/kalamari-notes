interface Props {
  vault: string | null;
  dark: boolean;
  view: "editor" | "graph";
  onOpenVault: () => void;
  onNewNote: () => void;
  onToggleTheme: () => void;
  onToggleView: () => void;
  onDailyNote: () => void;
  onSearch: () => void;
  onOpenPluginFolder: () => void;
}

export default function TopBar({
  vault,
  dark,
  view,
  onOpenVault,
  onNewNote,
  onToggleTheme,
  onToggleView,
  onDailyNote,
  onSearch,
  onOpenPluginFolder,
}: Props) {
  return (
    <header className="topbar">
      <h1>Kalamari</h1>
      <div className="topbar-actions">
        <button className="btn btn-primary" onClick={onNewNote}>
          New Note
        </button>
        <button className="btn btn-ghost" onClick={onDailyNote} disabled={!vault}>
          Daily
        </button>
        <button className="btn btn-ghost" onClick={onToggleView} disabled={!vault}>
          {view === "editor" ? "Graph" : "Editor"}
        </button>
        <button className="btn btn-ghost" onClick={onSearch}>
          Search
        </button>
        <button className="btn btn-ghost" onClick={onOpenVault}>
          {vault ? "Switch Vault" : "Open Vault"}
        </button>
        <button className="btn btn-ghost" onClick={onOpenPluginFolder} title="Open plugin folder">
          Plugins
        </button>
        <button className="btn btn-ghost" onClick={onToggleTheme}>
          {dark ? "Light" : "Dark"}
        </button>
      </div>
    </header>
  );
}
