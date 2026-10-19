import type { InstanceDetail, InstanceListItem } from "./types";

const BASE_URL = "";

export async function fetchInstances(): Promise<InstanceListItem[]> {
  const res = await fetch(`${BASE_URL}/api/instances`);
  if (!res.ok) {
    throw new Error(`Błąd pobierania listy instancji: ${res.statusText}`);
  }
  return res.json();
}

export async function fetchInstanceDetails(id: string): Promise<InstanceDetail> {
  const res = await fetch(`${BASE_URL}/api/instances/${encodeURIComponent(id)}`);
  if (!res.ok) {
    throw new Error(`Błąd pobierania szczegółów instancji ${id}: ${res.statusText}`);
  }
  return res.json();
}

export async function uploadInstanceXml(file: File): Promise<{ status: string; imported_ids: string[] }> {
  const formData = new FormData();
  formData.append("file", file);
  const res = await fetch(`${BASE_URL}/api/instances/upload`, {
    method: "POST",
    body: formData,
  });
  if (!res.ok) {
    const errorData = await res.json().catch(() => ({ detail: res.statusText }));
    throw new Error(errorData.detail || `Błąd importu pliku XML: ${res.statusText}`);
  }
  return res.json();
}

export function getExportInstanceUrl(id: string): string {
  return `${BASE_URL}/api/instances/${encodeURIComponent(id)}/export`;
}

export async function saveInstance(instance: InstanceDetail): Promise<{ status: string; id: string; message: string }> {
  const res = await fetch(`${BASE_URL}/api/instances`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(instance),
  });
  if (!res.ok) {
    const errorData = await res.json().catch(() => ({ detail: res.statusText }));
    throw new Error(errorData.detail || `Błąd zapisu instancji: ${res.statusText}`);
  }
  return res.json();
}

export async function deleteInstance(id: string): Promise<{ status: string; message: string }> {
  const res = await fetch(`${BASE_URL}/api/instances/${encodeURIComponent(id)}`, {
    method: "DELETE",
  });
  if (!res.ok) {
    const errorData = await res.json().catch(() => ({ detail: res.statusText }));
    throw new Error(errorData.detail || `Błąd usuwania instancji: ${res.statusText}`);
  }
  return res.json();
}
