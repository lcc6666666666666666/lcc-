import { spawn } from 'node:child_process';
import path from 'node:path';

const viteEntry = path.join(process.cwd(), 'node_modules', 'vite', 'bin', 'vite.js');

const commands = [
  ['node', ['server/index.mjs'], 'api'],
  [process.execPath, [viteEntry, '--host', '127.0.0.1'], 'vite'],
];

const children = commands.map(([command, args, name]) => {
  const child = spawn(command, args, {
    cwd: process.cwd(),
    shell: false,
    stdio: 'pipe',
    windowsHide: true,
  });

  child.stdout.on('data', (chunk) => process.stdout.write(`[${name}] ${chunk}`));
  child.stderr.on('data', (chunk) => process.stderr.write(`[${name}] ${chunk}`));
  child.on('exit', (code) => {
    if (code && code !== 0) {
      shutdown(code);
    }
  });

  return child;
});

process.on('SIGINT', () => shutdown(0));
process.on('SIGTERM', () => shutdown(0));

function shutdown(code) {
  for (const child of children) {
    if (!child.killed) {
      child.kill();
    }
  }
  process.exit(code);
}
