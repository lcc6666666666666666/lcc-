import MonacoEditor from '@monaco-editor/react';
import { registerMipsLanguage } from '../mipsLanguage';

interface ReadonlyCodePaneProps {
  value: string;
  language?: 'mips' | 'plaintext';
  emptyText?: string;
}

export default function ReadonlyCodePane({ value, language = 'plaintext', emptyText = 'No output yet' }: ReadonlyCodePaneProps) {
  if (!value.trim()) {
    return <div className="empty-state">{emptyText}</div>;
  }

  return (
    <MonacoEditor
      value={value}
      language={language}
      theme="snl-dark"
      beforeMount={(monaco) => {
        registerMipsLanguage(monaco);
      }}
      options={{
        automaticLayout: true,
        contextmenu: true,
        domReadOnly: true,
        fontFamily: '"Cascadia Code", Consolas, "Courier New", monospace',
        fontSize: 13,
        lineNumbers: 'on',
        minimap: { enabled: false },
        readOnly: true,
        scrollBeyondLastLine: false,
        wordWrap: 'on',
      }}
    />
  );
}
