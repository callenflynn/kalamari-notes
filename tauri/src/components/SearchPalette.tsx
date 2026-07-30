import { useEffect, useRef, useState } from "react";
import { invoke } from "@tauri-apps/api/core";

interface Props {
  vault: string | null;
  isOpen: boolean;
  onClose: () => void;
  onOpen: (path: string) => void;
}

export default function SearchPalette({ vault, isOpen, onClose, onOpen }: Props) {
  const [query, setQuery] = useState("");
  const [results, setResults] = useState<string[]>([]);
  const [loading, setLoading] = useState(false);
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    if (isOpen) {
      setTimeout(() => inputRef.current?.focus(), 50);
    }
  }, [isOpen]);

  useEffect(() => {
    if (!isOpen || !vault) {
      setResults([]);
      return;
    }
    if (query.length < 2) {
      setResults([]);
      return;
    }
    let cancelled = false;
    setLoading(true);
    const timer = setTimeout(() => {
      invoke<string[]>("search_vault", { vaultPath: vault, query })
        .then((res) => {
          if (!cancelled) setResults(res);
        })
        .finally(() => {
          if (!cancelled) setLoading(false);
        });
    }, 200);
    return () => {
      cancelled = true;
      clearTimeout(timer);
    };
  }, [isOpen, vault, query]);

  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      if (e.key === "Escape" && isOpen) {
        onClose();
      }
    };
    window.addEventListener("keydown", handler);
    return () => window.removeEventListener("keydown", handler);
  }, [isOpen, onClose]);

  if (!isOpen) return null;

  return (
    <div className="search-overlay" onClick={onClose}>
      <div className="search-modal" onClick={(e) => e.stopPropagation()}>
        <input
          ref={inputRef}
          className="search-input"
          placeholder="Search vault..."
          value={query}
          onChange={(e) => setQuery(e.target.value)}
        />
        <div className="search-results">
          {loading && <div className="search-empty">Searching...</div>}
          {!loading && results.length === 0 && query.length < 2 && (
            <div className="search-hint">Type at least 2 characters</div>
          )}
          {!loading && results.length === 0 && query.length >= 2 && (
            <div className="search-empty">No results</div>
          )}
          {results.map((path) => (
            <button
              key={path}
              className="search-result"
              onClick={() => {
                onOpen(path);
                onClose();
              }}
            >
              {path}
            </button>
          ))}
        </div>
      </div>
    </div>
  );
}
