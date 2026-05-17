import type * as Monaco from 'monaco-editor';

let completionsRegistered = false;

export function registerSnlLanguage(monaco: typeof Monaco) {
  if (!monaco.languages.getLanguages().some((language) => language.id === 'snl')) {
    monaco.languages.register({ id: 'snl' });
    monaco.languages.setLanguageConfiguration('snl', {
      comments: {
        blockComment: ['{', '}'],
      },
      brackets: [
        ['{', '}'],
        ['[', ']'],
        ['(', ')'],
      ],
      autoClosingPairs: [
        { open: '{', close: '}' },
        { open: '[', close: ']' },
        { open: '(', close: ')' },
        { open: "'", close: "'" },
      ],
      surroundingPairs: [
        { open: "'", close: "'" },
        { open: '(', close: ')' },
        { open: '[', close: ']' },
      ],
      wordPattern: /[a-zA-Z][a-zA-Z0-9]*/,
    });

    monaco.languages.setMonarchTokensProvider('snl', {
      keywords: [
        'program',
        'type',
        'var',
        'procedure',
        'begin',
        'end',
        'array',
        'of',
        'record',
        'if',
        'then',
        'else',
        'fi',
        'while',
        'do',
        'endwh',
        'read',
        'write',
        'return',
        'integer',
        'char',
      ],
      tokenizer: {
        root: [
          [/\{/, { token: 'comment', next: '@comment' }],
          [/'([^'\\]|\\.)*'/, 'string'],
          [/\d+/, 'number'],
          [/[a-zA-Z][a-zA-Z0-9]*/, { cases: { '@keywords': 'keyword', '@default': 'identifier' } }],
          [/:=|\.\.|[=<>+\-*/.;,()[\]]/, 'operator'],
          [/\s+/, 'white'],
        ],
        comment: [
          [/[^}]+/, 'comment'],
          [/\}/, { token: 'comment', next: '@pop' }],
        ],
      },
    });
  }

  monaco.editor.defineTheme('snl-dark', {
    base: 'vs-dark',
    inherit: true,
    rules: [
      { token: 'keyword', foreground: '569cd6' },
      { token: 'identifier', foreground: 'd4d4d4' },
      { token: 'number', foreground: 'b5cea8' },
      { token: 'string', foreground: 'ce9178' },
      { token: 'comment', foreground: '6a9955' },
      { token: 'operator', foreground: 'd4d4d4' },
    ],
    colors: {
      'editor.background': '#1e1e1e',
      'editor.foreground': '#d4d4d4',
      'editorLineNumber.foreground': '#858585',
      'editorCursor.foreground': '#d4d4d4',
      'editor.selectionBackground': '#264f78',
      'editor.inactiveSelectionBackground': '#3a3d41',
    },
  });

  if (!completionsRegistered) {
    completionsRegistered = true;
    monaco.languages.registerCompletionItemProvider('snl', {
      triggerCharacters: ['p', 'i', 'w', 'a', 'r'],
      provideCompletionItems(model, position) {
        const word = model.getWordUntilPosition(position);
        const range = {
          startLineNumber: position.lineNumber,
          endLineNumber: position.lineNumber,
          startColumn: word.startColumn,
          endColumn: word.endColumn,
        };

        return {
          suggestions: [
            snippet(monaco, 'program skeleton', 'program ${1:name}\nvar\n  integer ${2:x};\nbegin\n  ${3:write($2)}\nend.', range),
            snippet(monaco, 'if then else fi', 'if ${1:condition}\nthen\n  ${2:statement}\nelse\n  ${3:statement}\nfi', range),
            snippet(monaco, 'while do endwh', 'while ${1:condition} do\n  ${2:statement}\nendwh', range),
            snippet(monaco, 'procedure', 'procedure ${1:name}(${2:integer arg});\nvar integer ${3:i};\nbegin\n  ${4:statement}\nend', range),
            snippet(monaco, 'array type', 'type\n  ${1:Name}=array [${2:1}..${3:10}] of ${4:integer};', range),
            snippet(monaco, 'read(...)', 'read(${1:identifier})', range),
            snippet(monaco, 'write(...)', 'write(${1:identifier})', range),
          ],
        };
      },
    });
  }
}

function snippet(monaco: typeof Monaco, label: string, insertText: string, range: Monaco.IRange) {
  return {
    label,
    kind: monaco.languages.CompletionItemKind.Snippet,
    insertText,
    insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
    range,
  };
}
