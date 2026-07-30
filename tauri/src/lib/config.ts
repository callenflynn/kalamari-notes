import { invoke } from "@tauri-apps/api/core";

export interface AppConfig {
  lastVault?: string;
  darkMode?: boolean;
  sidebarWidth?: number;
}

export async function loadConfig(): Promise<AppConfig> {
  try {
    return (await invoke("load_config")) as AppConfig;
  } catch {
    return {};
  }
}

export async function saveConfig(config: AppConfig): Promise<void> {
  await invoke("save_config", { config });
}
