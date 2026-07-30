interface Props {
  title: string;
  subtitle?: string;
  hint?: string;
}

export default function EmptyState({ title, subtitle, hint }: Props) {
  return (
    <div className="empty-state">
      <h2>{title}</h2>
      {subtitle && <p>{subtitle}</p>}
      {hint && <p>{hint}</p>}
    </div>
  );
}
