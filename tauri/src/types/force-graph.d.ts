declare module "react-force-graph-2d" {
  import * as React from "react";

  interface ForceGraphProps {
    graphData: any;
    width?: number;
    height?: number;
    backgroundColor?: string;
    nodeColor?: (node: any) => string;
    nodeVal?: (node: any) => number;
    linkColor?: (link: any) => string;
    linkWidth?: number;
    linkDirectionalArrowLength?: number;
    linkDirectionalArrowRelPos?: number;
    onNodeClick?: (node: any) => void;
    onNodeHover?: (node: any) => void;
    nodeLabel?: (node: any) => string;
    nodeCanvasObject?: (node: any, ctx: CanvasRenderingContext2D, globalScale: number) => void;
    warmupTicks?: number;
    cooldownTicks?: number;
  }

  const ForceGraph2D: React.FC<ForceGraphProps>;
  export default ForceGraph2D;
}
