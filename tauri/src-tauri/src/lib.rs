use std::collections::{HashMap, HashSet};
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::OnceLock;

use regex::Regex;
use serde::{Deserialize, Serialize};
use tauri::Emitter;

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct VaultEntry {
    pub path: String,
    pub name: String,
    pub is_dir: bool,
    pub children: Vec<VaultEntry>,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct AppConfig {
    pub last_vault: Option<String>,
    pub dark_mode: bool,
    pub sidebar_width: u32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GraphNode {
    pub id: String,
    pub path: String,
    pub title: String,
    pub exists: bool,
    pub in_count: u32,
    pub out_count: u32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GraphEdge {
    pub source: String,
    pub target: String,
    pub kind: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Graph {
    pub nodes: Vec<GraphNode>,
    pub edges: Vec<GraphEdge>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Plugin {
    pub name: String,
    pub version: String,
    pub manifest: serde_json::Value,
    pub code: String,
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
pub struct AppState {
    watcher: std::sync::Mutex<Option<notify::RecommendedWatcher>>,
}

impl Default for AppState {
    fn default() -> Self {
        Self {
            watcher: std::sync::Mutex::new(None),
        }
    }
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

#[tauri::command]
fn list_vault(vault_path: String) -> Result<Vec<VaultEntry>, String> {
    let path = PathBuf::from(&vault_path);
    if !path.is_dir() {
        return Err(format!("Vault path is not a directory: {}", vault_path));
    }
    scan_dir(&path, &path)
}

fn scan_dir(base: &Path, current: &Path) -> Result<Vec<VaultEntry>, String> {
    let mut entries = vec![];
    let dir = fs::read_dir(current).map_err(|e| e.to_string())?;

    for entry in dir {
        let entry = entry.map_err(|e| e.to_string())?;
        let meta = entry.metadata().map_err(|e| e.to_string())?;
        let full = entry.path();
        let name = entry.file_name().into_string().map_err(|_| "invalid utf-8".to_string())?;

        if name.starts_with('.') {
            continue;
        }

        let rel = full.strip_prefix(base).map_err(|_| "path error".to_string())?;
        let rel_str = rel.to_string_lossy().replace("\\", "/");

        if meta.is_dir() {
            let children = scan_dir(base, &full)?;
            entries.push(VaultEntry {
                path: rel_str,
                name,
                is_dir: true,
                children,
            });
        } else if name.ends_with(".md") {
            entries.push(VaultEntry {
                path: rel_str,
                name,
                is_dir: false,
                children: vec![],
            });
        }
    }

    entries.sort_by(|a, b| {
        match (a.is_dir, b.is_dir) {
            (true, false) => std::cmp::Ordering::Less,
            (false, true) => std::cmp::Ordering::Greater,
            _ => a.name.to_lowercase().cmp(&b.name.to_lowercase()),
        }
    });

    Ok(entries)
}

#[tauri::command]
fn read_note(vault_path: String, note_path: String) -> Result<String, String> {
    let full = build_path(&vault_path, &note_path)?;
    fs::read_to_string(full).map_err(|e| e.to_string())
}

#[tauri::command]
fn write_note(vault_path: String, note_path: String, content: String) -> Result<(), String> {
    let full = build_path(&vault_path, &note_path)?;
    if let Some(parent) = Path::new(&full).parent() {
        fs::create_dir_all(parent).map_err(|e| e.to_string())?;
    }
    let tmp = full.with_extension("tmp");
    fs::write(&tmp, content).map_err(|e| e.to_string())?;
    fs::rename(&tmp, &full).map_err(|e| e.to_string())?;
    Ok(())
}

#[tauri::command]
fn create_note(vault_path: String, relative_dir: Option<String>, name: Option<String>) -> Result<String, String> {
    let base = PathBuf::from(&vault_path);
    let dir = match &relative_dir {
        Some(d) if !d.is_empty() => base.join(d),
        _ => base.clone(),
    };
    fs::create_dir_all(&dir).map_err(|e| e.to_string())?;

    let raw_name = match name {
        Some(n) if !n.is_empty() => n,
        _ => format!("{}.md", now_name()),
    };
    let file_name = sanitize_filename(&raw_name);
    if file_name.is_empty() {
        return Err("Invalid note name".to_string());
    }
    let file_name = if file_name.ends_with(".md") { file_name } else { format!("{}.md", file_name) };

    let full = dir.join(&file_name);
    if full.exists() {
        return Err("Note already exists".to_string());
    }

    let heading = file_name.trim_end_matches(".md").replace("-", " ").replace("_", " ");
    fs::write(&full, format!("# {}\n\n", heading)).map_err(|e| e.to_string())?;

    let rel = full.strip_prefix(&base).map_err(|_| "path error".to_string())?;
    Ok(rel.to_string_lossy().replace("\\", "/"))
}

#[tauri::command]
fn delete_note(vault_path: String, note_path: String) -> Result<(), String> {
    let full = build_path(&vault_path, &note_path)?;
    fs::remove_file(full).map_err(|e| e.to_string())
}

#[tauri::command]
fn rename_note(vault_path: String, note_path: String, new_name: String) -> Result<String, String> {
    let safe = sanitize_filename(&new_name);
    if safe.is_empty() {
        return Err("Invalid new name".to_string());
    }
    let safe = if safe.ends_with(".md") { safe } else { format!("{}.md", safe) };
    let full = build_path(&vault_path, &note_path)?;
    let new_full = if let Some(parent) = full.parent() {
        parent.join(safe)
    } else {
        return Err("Invalid note path".to_string());
    };
    let new_rel = new_full.strip_prefix(Path::new(&vault_path))
        .map_err(|_| "path error".to_string())?
        .to_string_lossy()
        .replace("\\", "/");
    fs::rename(full, new_full).map_err(|e| e.to_string())?;
    Ok(new_rel)
}

#[tauri::command]
fn search_vault(vault_path: String, query: String) -> Result<Vec<String>, String> {
    let base = PathBuf::from(&vault_path);
    let mut results = vec![];
    if query.len() < 2 {
        return Ok(results);
    }
    let q = query.to_lowercase();
    walk_and_search(&base, &base, &q, &mut results)?;
    Ok(results)
}

fn walk_and_search(base: &Path, current: &Path, query: &str, results: &mut Vec<String>) -> Result<(), String> {
    for entry in fs::read_dir(current).map_err(|e| e.to_string())? {
        let entry = entry.map_err(|e| e.to_string())?;
        let full = entry.path();
        let name = entry.file_name();
        if name.to_string_lossy().starts_with('.') {
            continue;
        }
        if entry.metadata().map_err(|e| e.to_string())?.is_dir() {
            walk_and_search(base, &full, query, results)?;
        } else if full.extension().and_then(|s| s.to_str()) == Some("md") {
            let rel = full.strip_prefix(base).map_err(|_| "path error".to_string())?;
            let rel_str = rel.to_string_lossy().replace("\\", "/");
            if rel_str.to_lowercase().contains(query) {
                results.push(rel_str);
                continue;
            }
            if let Ok(text) = fs::read_to_string(&full) {
                if text.to_lowercase().contains(query) {
                    results.push(rel_str);
                }
            }
        }
    }
    Ok(())
}

#[tauri::command]
fn load_config() -> AppConfig {
    let config_path = config_path();
    if let Ok(text) = fs::read_to_string(config_path) {
        if let Ok(cfg) = serde_json::from_str(&text) {
            return cfg;
        }
    }
    AppConfig::default()
}

#[tauri::command]
fn save_config(config: AppConfig) -> Result<(), String> {
    let config_path = config_path();
    if let Some(parent) = config_path.parent() {
        fs::create_dir_all(parent).map_err(|e| e.to_string())?;
    }
    let text = serde_json::to_string_pretty(&config).map_err(|e| e.to_string())?;
    fs::write(config_path, text).map_err(|e| e.to_string())
}

// ---------------------------------------------------------------------------
// Graph & Second brain
// ---------------------------------------------------------------------------

#[tauri::command]
fn get_vault_graph(vault_path: String) -> Result<Graph, String> {
    let base = PathBuf::from(&vault_path);
    let mut nodes: HashMap<String, GraphNode> = HashMap::new();
    let mut edges: Vec<GraphEdge> = vec![];

    let notes = scan_notes(&base, &base)?;
    let name_to_path: HashMap<String, String> = notes
        .iter()
        .map(|n| (n.name.trim_end_matches(".md").to_lowercase(), n.path.clone()))
        .collect();

    for note in &notes {
        nodes.entry(note.path.clone()).or_insert(GraphNode {
            id: note.path.clone(),
            path: note.path.clone(),
            title: note_title(&note.content, &note.name),
            exists: true,
            in_count: 0,
            out_count: 0,
        });
    }

    static WIKI_RE: OnceLock<Regex> = OnceLock::new();
    static MD_RE: OnceLock<Regex> = OnceLock::new();
    let wiki_re = WIKI_RE.get_or_init(|| Regex::new(r"\[\[([^\]]+)\]\]").unwrap());
    let md_re = MD_RE.get_or_init(|| Regex::new(r"\[([^\]]*)\]\(([^)\s]+)\)").unwrap());

    for note in &notes {
        for cap in wiki_re.captures_iter(&note.content) {
            let raw = cap.get(1).unwrap().as_str();
            let target = raw.split('|').next().unwrap_or(raw).trim();
            let target_path = resolve_link(target, &note.dir, &name_to_path);
            add_edge(&mut edges, &mut nodes, note.path.clone(), target_path, "wiki");
        }
        for cap in md_re.captures_iter(&note.content) {
            let url = cap.get(2).unwrap().as_str();
            if url.starts_with("http") || url.starts_with("mailto:") || url.starts_with('#') {
                continue;
            }
            let target_path = resolve_link(url, &note.dir, &name_to_path);
            add_edge(&mut edges, &mut nodes, note.path.clone(), target_path, "markdown");
        }
    }

    // recompute in/out counts
    for edge in &edges {
        if let Some(n) = nodes.get_mut(&edge.source) {
            n.out_count += 1;
        }
        if let Some(n) = nodes.get_mut(&edge.target) {
            n.in_count += 1;
        }
    }

    Ok(Graph {
        nodes: nodes.into_values().collect(),
        edges,
    })
}

#[tauri::command]
fn get_backlinks(vault_path: String, note_path: String) -> Result<Vec<String>, String> {
    let graph = get_vault_graph(vault_path)?;
    let mut seen = HashSet::new();
    for edge in graph.edges {
        if edge.target == note_path {
            seen.insert(edge.source);
        }
    }
    Ok(seen.into_iter().collect())
}

#[tauri::command]
fn create_wiki_note(vault_path: String, title: String) -> Result<String, String> {
    let base = PathBuf::from(&vault_path);
    let file_name = sanitize_filename(&title);
    if file_name.is_empty() {
        return Err("Invalid note title".to_string());
    }
    let file_name = format!("{}.md", file_name);
    let full = base.join(&file_name);

    if !full.exists() {
        let content = format!(
            "---\ntitle: \"{}\"\naliases: []\ncreated: {}\n---\n\n# {}\n\n",
            title,
            chrono::Local::now().format("%Y-%m-%d"),
            title
        );
        fs::write(&full, content).map_err(|e| e.to_string())?;
    }

    let rel = full.strip_prefix(&base).map_err(|_| "path error".to_string())?;
    Ok(rel.to_string_lossy().replace("\\", "/"))
}

#[tauri::command]
fn get_daily_note(vault_path: String) -> Result<String, String> {
    let base = PathBuf::from(&vault_path);
    let today = chrono::Local::now().format("%Y-%m-%d").to_string();
    let file_name = format!("{}.md", today);
    let full = base.join(&file_name);
    if !full.exists() {
        let content = format!("# {}\n\n", today);
        fs::write(&full, content).map_err(|e| e.to_string())?;
    }
    let rel = full.strip_prefix(&base).map_err(|_| "path error".to_string())?;
    Ok(rel.to_string_lossy().replace("\\", "/"))
}

// ---------------------------------------------------------------------------
// File watcher
// ---------------------------------------------------------------------------
#[tauri::command]
fn watch_vault(vault_path: String, app: tauri::AppHandle, state: tauri::State<AppState>) -> Result<(), String> {
    use notify::{Config, RecommendedWatcher, RecursiveMode, Watcher};

    // Drop the old watcher to release resources.
    {
        let mut lock = state.watcher.lock().map_err(|_| "mutex poisoned".to_string())?;
        *lock = None;
    }

    let mut watcher: RecommendedWatcher = RecommendedWatcher::new(
        move |res: Result<notify::Event, notify::Error>| {
            if let Ok(event) = res {
                match event.kind {
                    notify::EventKind::Create(_) | notify::EventKind::Modify(_) | notify::EventKind::Remove(_) => {
                        let _ = app.emit("vault-changed", ());
                    }
                    _ => {}
                }
            }
        },
        Config::default(),
    )
    .map_err(|e| e.to_string())?;

    watcher
        .watch(Path::new(&vault_path), RecursiveMode::Recursive)
        .map_err(|e| e.to_string())?;

    {
        let mut lock = state.watcher.lock().map_err(|_| "mutex poisoned".to_string())?;
        *lock = Some(watcher);
    }
    Ok(())
}

// ---------------------------------------------------------------------------
// Plugin loader
// ---------------------------------------------------------------------------
#[tauri::command]
fn open_plugin_folder() -> Result<(), String> {
    let dir = dirs::home_dir()
        .unwrap_or_else(|| PathBuf::from("."))
        .join(".kalamari")
        .join("plugins");
    fs::create_dir_all(&dir).map_err(|e| e.to_string())?;
    tauri_plugin_opener::open_path(&dir, None::<&str>).map_err(|e| e.to_string())
}

#[tauri::command]
fn load_plugins() -> Result<Vec<Plugin>, String> {
    let plugins_dir = dirs::home_dir()
        .unwrap_or_else(|| PathBuf::from("."))
        .join(".kalamari")
        .join("plugins");

    if !plugins_dir.is_dir() {
        return Ok(vec![]);
    }

    let mut plugins = vec![];
    for entry in fs::read_dir(plugins_dir).map_err(|e| e.to_string())? {
        let entry = entry.map_err(|e| e.to_string())?;
        let dir = entry.path();
        let manifest_path = dir.join("plugin.json");
        let code_path = dir.join("index.js");
        if !manifest_path.exists() || !code_path.exists() {
            continue;
        }
        let manifest_text = fs::read_to_string(manifest_path).map_err(|e| e.to_string())?;
        let manifest: serde_json::Value = serde_json::from_str(&manifest_text).map_err(|e| e.to_string())?;
        let name = manifest
            .get("name")
            .and_then(|v| v.as_str())
            .unwrap_or("unknown")
            .to_string();
        let version = manifest
            .get("version")
            .and_then(|v| v.as_str())
            .unwrap_or("0.0.0")
            .to_string();
        let code = fs::read_to_string(code_path).map_err(|e| e.to_string())?;
        plugins.push(Plugin {
            name,
            version,
            manifest,
            code,
        });
    }
    Ok(plugins)
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
struct NoteMeta {
    path: String,
    name: String,
    dir: String,
    content: String,
}

fn scan_notes(base: &Path, current: &Path) -> Result<Vec<NoteMeta>, String> {
    let mut out = vec![];
    for entry in fs::read_dir(current).map_err(|e| e.to_string())? {
        let entry = entry.map_err(|e| e.to_string())?;
        let full = entry.path();
        let name_str = entry.file_name();
        let name_lossy = name_str.to_string_lossy();
        if name_lossy.starts_with('.') {
            continue;
        }
        if entry.metadata().map_err(|e| e.to_string())?.is_dir() {
            out.extend(scan_notes(base, &full)?);
        } else if full.extension().and_then(|s| s.to_str()) == Some("md") {
            let rel = full.strip_prefix(base).map_err(|_| "path error".to_string())?;
            let rel_str = rel.to_string_lossy().replace("\\", "/");
            let dir = Path::new(&rel_str)
                .parent()
                .map(|p| p.to_string_lossy().to_string())
                .unwrap_or_default();
            let content = fs::read_to_string(&full).unwrap_or_default();
            out.push(NoteMeta {
                path: rel_str,
                name: name_lossy.to_string(),
                dir,
                content,
            });
        }
    }
    Ok(out)
}

fn note_title(content: &str, fallback: &str) -> String {
    static TITLE_RE: OnceLock<Regex> = OnceLock::new();
    static HEADING_RE: OnceLock<Regex> = OnceLock::new();
    let title_re = TITLE_RE.get_or_init(|| Regex::new(r"(?m)^title:\s*(.+)").unwrap());
    if let Some(cap) = title_re.captures(content) {
        return cap.get(1).unwrap().as_str().trim().to_string();
    }
    let heading_re = HEADING_RE.get_or_init(|| Regex::new(r"(?m)^#\s+(.+)").unwrap());
    if let Some(cap) = heading_re.captures(content) {
        return cap.get(1).unwrap().as_str().trim().to_string();
    }
    fallback.trim_end_matches(".md").to_string()
}

fn resolve_link(target: &str, current_dir: &str, name_to_path: &HashMap<String, String>) -> String {
    let clean = target.trim().replace('\\', "/");
    if clean.contains('/') {
        let with_ext = if clean.ends_with(".md") { clean.clone() } else { format!("{}.md", clean) };
        return with_ext;
    }
    let key = clean.trim_end_matches(".md").to_lowercase();
    if let Some(path) = name_to_path.get(&key) {
        return path.clone();
    }
    if !current_dir.is_empty() {
        return format!("{}/{}.md", current_dir, sanitize_filename(&clean));
    }
    format!("{}.md", sanitize_filename(&clean))
}

fn add_edge(edges: &mut Vec<GraphEdge>, nodes: &mut HashMap<String, GraphNode>, source: String, target: String, kind: &str) {
    edges.push(GraphEdge {
        source: source.clone(),
        target: target.clone(),
        kind: kind.to_string(),
    });
    if !nodes.contains_key(&target) {
        nodes.insert(
            target.clone(),
            GraphNode {
                id: target.clone(),
                path: target.clone(),
                title: target.trim_end_matches(".md").replace('-', " ").replace('_', " "),
                exists: false,
                in_count: 0,
                out_count: 0,
            },
        );
    }
}

fn build_path(vault_path: &str, note_path: &str) -> Result<PathBuf, String> {
    let base = PathBuf::from(vault_path);
    let note = PathBuf::from(note_path.trim_start_matches(|c| c == '/' || c == '\\'));

    // Reject any attempt to escape the vault with parent/root components.
    if note.components().any(|c| matches!(c, std::path::Component::ParentDir | std::path::Component::RootDir)) {
        return Err("Invalid path: directory traversal is not allowed".to_string());
    }

    Ok(base.join(note))
}

fn config_path() -> PathBuf {
    dirs::home_dir()
        .unwrap_or_else(|| PathBuf::from("."))
        .join(".kalamari")
        .join("config.json")
}

fn sanitize_filename(name: &str) -> String {
    let cleaned: String = name
        .replace('/', "-")
        .replace('\\', "-")
        .replace(':', "-")
        .replace('*', "-")
        .replace('?', "-")
        .replace('"', "-")
        .replace('<', "-")
        .replace('>', "-")
        .replace('|', "-")
        .chars()
        .filter(|c| !c.is_control())
        .collect();
    cleaned.trim().trim_start_matches('.').to_string()
}

fn now_name() -> String {
    use std::time::SystemTime;
    let now = SystemTime::now()
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs();
    format!("note-{}", now)
}

// ---------------------------------------------------------------------------
// Entry
// ---------------------------------------------------------------------------
#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .plugin(tauri_plugin_dialog::init())
        .manage(AppState::default())
        .invoke_handler(tauri::generate_handler![
            list_vault,
            read_note,
            write_note,
            create_note,
            delete_note,
            rename_note,
            search_vault,
            load_config,
            save_config,
            get_vault_graph,
            get_backlinks,
            create_wiki_note,
            get_daily_note,
            watch_vault,
            open_plugin_folder,
            load_plugins
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
