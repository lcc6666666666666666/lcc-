import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import fs from 'node:fs/promises';
import path from 'node:path';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(scriptDir, '..', '..');
const outputDir = path.join(projectRoot, 'build', 'web-compiler');
const outputName = process.platform === 'win32' ? 'compiler.exe' : 'compiler';
const outputPath = path.join(outputDir, outputName);
const compilerOutputArg = path.join('build', 'web-compiler', outputName);

const sources = ['main.cpp', 'lexer.cpp', 'parser.cpp', 'semantic.cpp', 'midcode.cpp', 'codegen.cpp'];

async function ensureSourcesExist() {
  const missing = [];
  for (const source of sources) {
    try {
      await fs.access(path.join(projectRoot, source));
    } catch {
      missing.push(source);
    }
  }

  if (missing.length > 0) {
    throw new Error(`Missing compiler source files: ${missing.join(', ')}`);
  }
}

async function build() {
  await ensureSourcesExist();
  await fs.mkdir(outputDir, { recursive: true });

  const args = ['-std=c++17', '-g', ...sources, '-o', compilerOutputArg];
  console.log(`Building SNL compiler: g++ ${args.join(' ')}`);

  const code = await new Promise((resolve, reject) => {
    const child = spawn('g++', args, {
      cwd: projectRoot,
      shell: false,
      stdio: 'inherit',
      windowsHide: true,
    });

    child.on('error', (error) => {
      if (error.code === 'ENOENT') {
        reject(
          new Error(
            'g++ was not found. Install MinGW-w64/MSYS2 on Windows or GCC on Unix, then ensure g++ is available on PATH.',
          ),
        );
        return;
      }
      reject(error);
    });

    child.on('close', resolve);
  });

  if (code !== 0) {
    throw new Error(`g++ exited with code ${code}`);
  }

  console.log(`Compiler written to ${outputPath}`);
}

build().catch((error) => {
  console.error(error.message);
  process.exit(1);
});
