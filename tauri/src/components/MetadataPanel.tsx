import { useMemo } from "react";
import type { Graph, Tab } from "../lib/types";

interface Props {
  note: Tab | null;
  graph: Graph | null;
  onTagClick?: (tag: string) => void;
}

function parseFrontmatter(content: string): Record<string, unknown> | null {
  const match = content.trimStart().match(/^---\r?\n([\s\S]*?)\r?\n---/);
  if (!match) return null;
  const fm: Record<string, unknown> = {};
  match[1].split(/\r?\n/).forEach((line) => {
    const idx = line.indexOf(":");
    if (idx > 0) {
      const key = line.slice(0, idx).trim();
      let value: any = line.slice(idx + 1).trim();
      if (value.startsWith("[") && value.endsWith("]")) {
        value =      value
        .slice(1, -1)
        .split(",")
        .map((s: string) => s.trim().replace(/^["']|["']$/g, ""));
      } else if (value.startsWith("\"") && value.endsWith("\"")) {
        value = value.slice(1, -1);
      }
      fm[key] = value;
    }
  });
  return fm;
}

function extractInlineTags(content: string): string[] {
  const regex = /(?:^|\s)#([A-Za-z0-9_\-]+)/g;
  const tags = new Set<string>();
  let m: RegExpExecArray | null;
  while ((m = regex.exec(content)) !== null) {
    tags.add(m[1]);
  }
  return Array.from(tags).sort();
}

export default function MetadataPanel({ note, graph, onTagClick }: Props) {
  const frontmatter = useMemo(() => (note ? parseFrontmatter(note.content) : null), [note]);
  const inlineTags = useMemo(() => (note ? extractInlineTags(note.content) : []), [note]);
  const noteNode = useMemo(() => graph?.nodes.find((n) => n.path === note?.path), [graph, note]);

  if (!note) {
    return (
      <div className="info-panel">
        <h3>Metadata</h3>
        <p className="info-empty">Open a note to see metadata.</p>
      </div>
    );
  }

  const tags: string[] = [];
  const fmTags = frontmatter?.tags;
  if (Array.isArray(fmTags)) tags.push(...(fmTags as string[]));
  tags.push(...inlineTags);

  return (
    <div className="info-panel">
      <h3>Metadata</h3>
      <div className="meta-block">
        <span className="meta-label">Path</span>
        <span className="meta-value" title={note.path}>
          {note.path}
        </span>
      </div>
      {frontmatter &&
        Object.entries(frontmatter).map(([key, value]) =>
          key === "tags" ? null : (
            <div key={key} className="meta-block">
              <span className="meta-label">{key}</span>
              <span className="meta-value">{String(value)}</span>
            </div>
          )
        )}
      {tags.length > 0 && (
        <div className="meta-block">
          <span className="meta-label">Tags</span>
          <div className="tag-list">
            {tags.map((tag) => (
              <button key={tag} className="tag-pill" onClick={() => onTagClick?.(tag)}>
                #{tag}
              </button>
            ))}
          </div>
        </div>
      )}
      {noteNode && (
        <div className="meta-block">
          <span className="meta-label">Graph</span>
          <span className="meta-value">
            {noteNode.in_count} in / {noteNode.out_count} out
          </span>
        </div>
      )}
    </div>
  );
}
