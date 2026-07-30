import { useMemo, useRef, useState, useEffect } from "react";
import ForceGraph2D from "react-force-graph-2d";
import type { Graph } from "../lib/types";

interface Props {
  graph: Graph;
  dark: boolean;
  onNodeClick: (path: string) => void;
}

function useSize<T extends HTMLElement>() {
  const ref = useRef<T>(null);
  const [size, setSize] = useState({ width: 0, height: 0 });

  useEffect(() => {
    const el = ref.current;
    if (!el) return;
    const observer = new ResizeObserver((entries) => {
      for (const entry of entries) {
        const { width, height } = entry.contentRect;
        setSize({ width, height });
      }
    });
    observer.observe(el);
    return () => observer.disconnect();
  }, []);

  return { ref, size };
}

export default function GraphView({ graph, dark, onNodeClick }: Props) {
  const { ref, size } = useSize<HTMLDivElement>();
  const [hoverNode, setHoverNode] = useState<{ id: string; label: string } | null>(null);

  const data = useMemo(
    () => ({
      nodes: graph.nodes.map((n) => ({ ...n })),
      links: graph.edges.map((e) => ({ ...e })),
    }),
    [graph]
  );

  const colors = useMemo(() => {
    const root = getComputedStyle(document.documentElement);
    return {
      accent: root.getPropertyValue("--accent").trim() || "#ED5001",
      muted: root.getPropertyValue("--text-muted").trim() || "#8A8070",
      primary: root.getPropertyValue("--text-primary").trim() || "#242424",
      secondary: root.getPropertyValue("--text-secondary").trim() || "#5A4A3A",
    };
  }, [dark]);

  return (
    <div ref={ref} className="graph-view">
      {hoverNode && <div className="graph-tooltip">{hoverNode.label}</div>}
      {size.width > 0 && size.height > 0 && (
        <ForceGraph2D
          graphData={data as any}
          width={size.width}
          height={size.height}
          backgroundColor="transparent"
          nodeColor={(n: any) => (n.exists ? "var(--accent)" : "var(--text-muted)")}
          nodeVal={(n: any) => Math.max(3, (n.in_count || 0) + (n.out_count || 0))}
          linkColor={() => "var(--border-strong)"}
          linkWidth={1}
          linkDirectionalArrowLength={4}
          linkDirectionalArrowRelPos={1}
          onNodeClick={(n: any) => onNodeClick(n.path as string)}
          onNodeHover={(n: any) => setHoverNode(n ? { id: n.id, label: `${n.title} (${n.in_count} in / ${n.out_count} out)` } : null)}
          nodeLabel={(n: any) => `${n.title}\n${n.in_count} in / ${n.out_count} out`}
          nodeCanvasObject={(node: any, ctx: CanvasRenderingContext2D, globalScale: number) => {
            const size = Math.max(4, ((node.in_count || 0) + (node.out_count || 0) + 2) * 1.5);
            const x = node.x || 0;
            const y = node.y || 0;

            ctx.beginPath();
            ctx.arc(x, y, size / globalScale, 0, 2 * Math.PI);
            ctx.fillStyle = node.exists ? colors.accent : colors.muted;
            ctx.fill();
            if (hoverNode && node.id === hoverNode.id) {
              ctx.strokeStyle = colors.primary;
              ctx.lineWidth = 2 / globalScale;
              ctx.stroke();
            }

            const label = node.title || node.id;
            ctx.font = `${12 / globalScale}px system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif`;
            ctx.fillStyle = colors.secondary;
            ctx.textAlign = "center";
            ctx.fillText(label, x, y + size / globalScale + 12 / globalScale);
          }}
          warmupTicks={10}
          cooldownTicks={50}
        />
      )}
    </div>
  );
}
