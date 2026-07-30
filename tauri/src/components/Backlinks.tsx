import { useEffect, useState } from "react";
import { invoke } from "@tauri-apps/api/core";

interface Props {
  vault: string | null;
  notePath: string | null;
  onOpen: (path: string) => void;
}

export default function Backlinks({ vault, notePath, onOpen }: Props) {
  const [links, setLinks] = useState<string[]>([]);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    if (!vault || !notePath) {
      setLinks([]);
      return;
    }
    let cancelled = false;
    setLoading(true);
    invoke<string[]>("get_backlinks", { vaultPath: vault, notePath })
      .then((res) => {
        if (!cancelled) setLinks(res);
      })
      .finally(() => {
        if (!cancelled) setLoading(false);
      });
    return () => {
      cancelled = true;
    };
  }, [vault, notePath]);

  if (!notePath) {
    return (
      <div className="info-panel">
        <h3>Backlinks</h3>
        <p className="info-empty">Open a note to see backlinks.</p>
      </div>
    );
  }

  return (
    <div className="info-panel">
      <h3>Backlinks</h3>
      {loading ? (
        <p className="info-empty">Loading...</p>
      ) : links.length === 0 ? (
        <p className="info-empty">No notes link here yet.</p>
      ) : (
        <ul className="info-list">
          {links.map((path) => (
            <li key={path}>
              <button className="link-button" onClick={() => onOpen(path)}>
                {path}
              </button>
            </li>
          ))}
        </ul>
      )}
    </div>
  );
}
