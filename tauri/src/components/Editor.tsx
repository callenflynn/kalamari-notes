import { useState } from "react";
import type { Note } from "../lib/types";
import MarkdownPreview from "./MarkdownPreview";

interface Props {
  note: Note;
  onChange: (content: string) => void;
  onSave: () => void;
  onWikiLink?: (target: string) => void;
}

export default function Editor({ note, onChange, onSave, onWikiLink }: Props) {
  const [preview, setPreview] = useState(true);

  return (
    <div className="editor-pane">
      <div className="editor-toolbar">
        <span className="editor-toolbar-title">{note.path}</span>
        <div className="editor-toolbar-actions">
          <button className="btn btn-ghost" onClick={() => setPreview((p) => !p)}>
            {preview ? "Edit Only" : "Preview"}
          </button>
          {note.dirty && (
            <button className="btn btn-primary" onClick={onSave}>
              Save
            </button>
          )}
        </div>
      </div>
      <div
        className="editor-area"
        style={{ gridTemplateColumns: preview ? "1fr 1fr" : "1fr" }}
      >
        <textarea
          className="editor-textarea"
          value={note.content}
          onChange={(e) => onChange(e.target.value)}
          placeholder="Start writing..."
          spellCheck={false}
        />
        {preview && <MarkdownPreview content={note.content} onWikiLink={onWikiLink} />}
      </div>
    </div>
  );
}
