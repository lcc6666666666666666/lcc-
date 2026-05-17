import type { CompileResponse } from './types';

export async function compileSource(source: string): Promise<CompileResponse> {
  let response: Response;

  try {
    response = await fetch('/api/compile', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ source }),
    });
  } catch (error) {
    throw new Error(error instanceof Error ? `Cannot reach compiler server: ${error.message}` : 'Cannot reach compiler server.');
  }

  let payload: CompileResponse | undefined;
  try {
    payload = await response.json();
  } catch {
    throw new Error(`Compiler server returned HTTP ${response.status}.`);
  }

  if (!response.ok) {
    throw new Error(payload.message || `Compiler server returned HTTP ${response.status}.`);
  }

  return payload;
}
