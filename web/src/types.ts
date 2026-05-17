export type CompileStage = 'lexer' | 'parser' | 'semantic' | 'codegen' | 'compiler';
export type CompileStatus = 'idle' | 'compiling' | 'success' | 'error';
export type BottomTab = 'problems' | 'tokens' | 'ast' | 'mips' | 'console' | 'raw';
export type SourceFileKey = 'source' | 'token' | 'tree' | 'mips' | 'error';

export interface Diagnostic {
  line: number;
  column: number;
  message: string;
  stage: CompileStage;
  severity: 'error' | 'warning' | 'info';
}

export interface CompilerOutputs {
  token: string;
  tree: string;
  mips: string;
  error: string;
  stdout: string;
  stderr: string;
}

export interface CompileResponse {
  ok: boolean;
  durationMs: number;
  outputs: CompilerOutputs;
  diagnostics: Diagnostic[];
  message?: string;
}

export interface TokenRow {
  index: number;
  line: number;
  type: string;
  lexeme: string;
}

export interface AstNode {
  id: string;
  label: string;
  type: string;
  level: number;
  children: AstNode[];
}
