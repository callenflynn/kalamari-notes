import { Tab } from "../lib/types";

interface Props {
  tabs: Tab[];
  active: string | null;
  onSelect: (path: string) => void;
  onClose: (path: string) => void;
}

export default function TabBar({ tabs, active, onSelect, onClose }: Props) {
  return (
    <div className="tabs">
      {tabs.map((tab) => (
        <div
          key={tab.path}
          className={`tab ${active === tab.path ? "active" : ""}`}
          onClick={() => onSelect(tab.path)}
        >
          <span>{tab.path.split("/").pop()}</span>
          <span
            className="tab-close"
            onClick={(e) => {
              e.stopPropagation();
              onClose(tab.path);
            }}
          >
            ×
          </span>
        </div>
      ))}
    </div>
  );
}
