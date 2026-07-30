import { visit } from "unist-util-visit";
import type { Node } from "unist";

interface WikiLinkOptions {
  onClick: (target: string) => void;
}

export function wikiLinkPlugin(options: WikiLinkOptions) {
  return (tree: Node) => {
    visit(tree, "text", (node: any, index: number | undefined, parent: any) => {
      if (!parent || typeof index !== "number") return;
      const text: string = node.value;
      const regex = /\[\[([^\]]+)\]\]/g;
      const children: any[] = [];
      let lastIndex = 0;
      let match: RegExpExecArray | null;

      while ((match = regex.exec(text)) !== null) {
        const before = text.slice(lastIndex, match.index);
        if (before) {
          children.push({ type: "text", value: before });
        }

        const inner = match[1];
        const [target, alias] = inner.split("|").map((s) => s.trim());
        children.push({
          type: "link",
          url: `#wiki:${target}`,
          data: {
            hProperties: {
              className: "wiki-link",
              "data-target": target,
              onClick: (e: MouseEvent) => {
                e.preventDefault();
                options.onClick(target);
              },
            },
          },
          children: [{ type: "text", value: alias || target }],
        });
        lastIndex = regex.lastIndex;
      }

      if (children.length > 0) {
        const after = text.slice(lastIndex);
        if (after) children.push({ type: "text", value: after });
        parent.children.splice(index, 1, ...children);
      }
    });
  };
}
