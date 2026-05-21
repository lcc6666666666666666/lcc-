import express from 'express';
import { spawn } from 'node:child_process';
import { randomUUID } from 'node:crypto';
import fs from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import { performance } from 'node:perf_hooks';
import { fileURLToPath } from 'node:url';

const PORT = Number(process.env.PORT || 3001);
const SOURCE_LIMIT_BYTES = 200 * 1024;
const RUN_TIMEOUT_MS = 5000;
const MAX_CAPTURE_BYTES = 1024 * 1024;

const serverDir = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(serverDir, '..', '..');

const app = express();

app.use(express.json({ limit: '220kb' }));

app.post('/api/compile', async (req, res) => {
  const start = performance.now();
  let tempDir = '';
  let outputs = emptyOutputs();

  try {
    const source = req.body?.source;
    if (typeof source !== 'string') {
      return res.status(400).json(failure(start, outputs, [], 'Request body must contain a string field named source.'));
    }

    if (Buffer.byteLength(source, 'utf8') > SOURCE_LIMIT_BYTES) {
      return res.status(413).json(failure(start, outputs, [], 'Source is larger than the 200KB limit.'));
    }

    const compilerPath = getCompilerPath();
    await fs.access(compilerPath);

    tempDir = path.join(os.tmpdir(), `snl-compiler-${randomUUID()}`);
    await fs.mkdir(tempDir, { recursive: false });
    await fs.writeFile(path.join(tempDir, 'source.txt'), source, 'utf8');

    const run = await runCompiler(compilerPath, tempDir);
    outputs = await collectOutputs(tempDir, run.stdout, run.stderr);
    const diagnostics = parseDiagnostics(outputs);

    if (run.timedOut) {
      return res.status(200).json(
        failure(start, outputs, diagnostics, `Compiler timed out after ${RUN_TIMEOUT_MS}ms.`),
      );
    }

    if (run.exitCode !== 0) {
      return res.status(200).json(
        failure(start, outputs, diagnostics, `Compiler exited with code ${run.exitCode}.`),
      );
    }

    return res.json({
      ok: true,
      durationMs: elapsedMs(start),
      outputs,
      diagnostics,
    });
  } catch (error) {
    if (tempDir) {
      outputs = await collectOutputs(tempDir, outputs.stdout, outputs.stderr);
    }
    const diagnostics = parseDiagnostics(outputs);
    return res.status(200).json(
      failure(start, outputs, diagnostics, error instanceof Error ? error.message : 'Compile failed.'),
    );
  } finally {
    if (tempDir) {
      await fs.rm(tempDir, { recursive: true, force: true }).catch(() => {});
    }
  }
});

app.use((error, _req, res, _next) => {
  const message = error?.type === 'entity.too.large'
    ? 'Source is larger than the 200KB limit.'
    : 'Invalid JSON request body.';
  res.status(error?.type === 'entity.too.large' ? 413 : 400).json({
    ok: false,
    durationMs: 0,
    outputs: emptyOutputs(),
    diagnostics: [],
    message,
  });
});

app.listen(PORT, () => {
  console.log(`SNL compiler API listening on http://localhost:${PORT}`);
});

function getCompilerPath() {
  if (process.env.COMPILER_BIN) {
    return path.isAbsolute(process.env.COMPILER_BIN)
      ? process.env.COMPILER_BIN
      : path.resolve(process.cwd(), process.env.COMPILER_BIN);
  }

  const executable = process.platform === 'win32' ? 'compiler.exe' : 'compiler';
  return path.join(projectRoot, 'build', 'web-compiler', executable);
}

function runCompiler(compilerPath, cwd) {
  return new Promise((resolve, reject) => {
    let stdout = '';
    let stderr = '';
    let stdoutBytes = 0;
    let stderrBytes = 0;
    let timedOut = false;
    let settled = false;

    const child = spawn(compilerPath, [], {
      cwd,
      shell: false,
      windowsHide: true,
    });

    const timer = setTimeout(() => {
      timedOut = true;
      child.kill();
    }, RUN_TIMEOUT_MS);

    child.stdout?.on('data', (chunk) => {
      if (stdoutBytes >= MAX_CAPTURE_BYTES) {
        return;
      }
      const text = chunk.toString('utf8');
      stdoutBytes += Buffer.byteLength(text, 'utf8');
      stdout += text;
    });

    child.stderr?.on('data', (chunk) => {
      if (stderrBytes >= MAX_CAPTURE_BYTES) {
        return;
      }
      const text = chunk.toString('utf8');
      stderrBytes += Buffer.byteLength(text, 'utf8');
      stderr += text;
    });

    child.on('error', (error) => {
      clearTimeout(timer);
      if (!settled) {
        settled = true;
        reject(error);
      }
    });

    child.on('close', (exitCode, signal) => {
      clearTimeout(timer);
      if (!settled) {
        settled = true;
        resolve({ exitCode, signal, timedOut, stdout, stderr });
      }
    });
  });
}

async function collectOutputs(tempDir, stdout = '', stderr = '') {
  return {
    token: await readText(path.join(tempDir, 'token.txt')),
    tree: await readText(path.join(tempDir, 'tree.txt')),
    midcode: await readText(path.join(tempDir, 'midcode.txt')),
    mips: await readText(path.join(tempDir, 'mips.txt')),
    error: await readText(path.join(tempDir, 'error.txt')),
    stdout,
    stderr,
  };
}

async function readText(filePath) {
  try {
    return await fs.readFile(filePath, 'utf8');
  } catch {
    return '';
  }
}

function emptyOutputs() {
  return {
    token: '',
    tree: '',
    midcode: '',
    mips: '',
    error: '',
    stdout: '',
    stderr: '',
  };
}

function failure(start, outputs, diagnostics, message) {
  return {
    ok: false,
    durationMs: elapsedMs(start),
    outputs,
    diagnostics,
    message,
  };
}

function elapsedMs(start) {
  return Math.max(0, Math.round(performance.now() - start));
}

function parseDiagnostics(outputs) {
  const diagnostics = [];

  for (const line of outputs.error.split(/\r?\n/)) {
    const trimmed = line.trim();
    if (!trimmed) {
      continue;
    }
    const match = trimmed.match(/^(\d+)\s*行\s*(.+)$/);
    if (match) {
      diagnostics.push({
        line: Number(match[1]),
        column: 1,
        message: match[2].trim(),
        stage: 'lexer',
        severity: 'error',
      });
    } else {
      diagnostics.push({
        line: 1,
        column: 1,
        message: trimmed,
        stage: 'lexer',
        severity: 'error',
      });
    }
  }

  for (const text of [outputs.stdout, outputs.stderr]) {
    for (const line of text.split(/\r?\n/)) {
      const trimmed = line.trim();
      if (!trimmed) {
        continue;
      }
      const match = trimmed.match(/\bline\s+(\d+)\s*:?\s*(.+)$/i);
      if (!match) {
        continue;
      }
      const message = match[2].trim();
      diagnostics.push({
        line: Number(match[1]),
        column: 1,
        message,
        stage: inferStage(message),
        severity: 'error',
      });
    }
  }

  const seen = new Set();
  return diagnostics.filter((diagnostic) => {
    const key = `${diagnostic.line}:${diagnostic.column}:${diagnostic.stage}:${diagnostic.message}`;
    if (seen.has(key)) {
      return false;
    }
    seen.add(key);
    return true;
  });
}

function inferStage(message) {
  if (/syntax|missing/i.test(message)) {
    return 'parser';
  }
  if (/未声明|重复定义|类型|语义|semantic/i.test(message)) {
    return 'semantic';
  }
  if (/codegen|mips|assembly/i.test(message)) {
    return 'codegen';
  }
  return 'compiler';
}
