import type * as Monaco from 'monaco-editor';

export function registerMipsLanguage(monaco: typeof Monaco) {
  if (!monaco.languages.getLanguages().some((language) => language.id === 'mips')) {
    monaco.languages.register({ id: 'mips' });
    monaco.languages.setMonarchTokensProvider('mips', {
      instructions: [
        'add',
        'sub',
        'addi',
        'li',
        'lw',
        'sw',
        'move',
        'j',
        'jal',
        'jr',
        'beq',
        'bne',
        'blt',
        'ble',
        'bgt',
        'bge',
        'syscall',
        'mult',
        'mflo',
        'la',
      ],
      tokenizer: {
        root: [
          [/#.*$/, 'comment'],
          [/^[A-Za-z_.$][\w.$]*:/, 'type.identifier'],
          [/\$[a-zA-Z0-9]+/, 'variable.predefined'],
          [/[a-zA-Z][a-zA-Z0-9]*/, { cases: { '@instructions': 'keyword', '@default': 'identifier' } }],
          [/-?\d+/, 'number'],
          [/[(),]/, 'delimiter'],
          [/\s+/, 'white'],
        ],
      },
    });
  }
}
