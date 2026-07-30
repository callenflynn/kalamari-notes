import { useCallback } from "react";
import ReactMarkdown from "react-markdown";
import remarkGfm from "remark-gfm";
import { wikiLinkPlugin } from "../lib/wikiLinkPlugin";

interface Props {
  content: string;
  onWikiLink?: (target: string) => void;
}

export default function MarkdownPreview({ content, onWikiLink }: Props) {
  const LinkComponent = useLinkComponent(onWikiLink);
  return (
    <div className="preview-pane">
      <ReactMarkdown
        remarkPlugins={[remarkGfm, wikiLinkPlugin({ onClick: onWikiLink || (() => {}) })]}
        components={{ a: LinkComponent }}
      >
        {content}
      </ReactMarkdown>
    </div>
  );
}

function useLinkComponent(onWikiLink?: (target: string) => void) {
  return useCallback(
    ({ node, children, ...props }: any) => {
      const target =
        node?.properties?.dataTarget ||
        node?.properties?.["data-target"] ||
        props.dataTarget ||
        props["data-target"];
      if (target) {
        return (
          <a
            href="#"
            className="wiki-link"
            onClick={(e) => {
              e.preventDefault();
              onWikiLink?.(target as string);
            }}
          >
            {children}
          </a>
        );
      }
      return <a {...props}>{children}</a>;
    },
    [onWikiLink]
  );
}
