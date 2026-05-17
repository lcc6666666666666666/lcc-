import { useState } from 'react';
import type { AstNode } from '../types';

interface AstTreeProps {
  nodes: AstNode[];
  rawText: string;
}

const nodeKinds = ['ProK', 'PheadK', 'TypeK', 'VarK', 'ProcDecK', 'StmtK', 'ExpK', 'DecK', 'StmLK'];

export function parseAstTree(treeText: string): AstNode[] {
  const roots: AstNode[] = [];
  const stack: AstNode[] = [];

  treeText.split(/\r?\n/).forEach((line, index) => {
    const levelMatch = line.match(/\(Level:\s*(\d+)\)\s*$/);
    if (!levelMatch) {
      return;
    }

    const level = Number(levelMatch[1]);
    const label = cleanLabel(line.replace(/\(Level:\s*\d+\)\s*$/, ''));
    if (!label) {
      return;
    }

    const node: AstNode = {
      id: `${index}-${level}-${label}`,
      label,
      type: detectNodeType(label),
      level,
      children: [],
    };

    stack[level] = node;
    stack.length = level + 1;

    if (level === 0) {
      roots.push(node);
      return;
    }

    const parent = stack[level - 1] ?? roots[roots.length - 1];
    if (parent) {
      parent.children.push(node);
    } else {
      roots.push(node);
    }
  });

  return roots;
}

export default function AstTree({ nodes, rawText }: AstTreeProps) {
  if (!rawText.trim()) {
    return <div className="empty-state">No AST yet</div>;
  }

  if (nodes.length === 0) {
    return <pre className="raw-pre">{rawText}</pre>;
  }

  return (
    <div className="ast-tree">
      {nodes.map((node) => (
        <TreeNode key={node.id} node={node} />
      ))}
    </div>
  );
}

function TreeNode({ node }: { node: AstNode }) {
  const [expanded, setExpanded] = useState(true);
  const hasChildren = node.children.length > 0;

  return (
    <div className="ast-node">
      <div className="ast-node__row" style={{ paddingLeft: `${node.level * 14}px` }}>
        <button
          className="ast-node__toggle"
          type="button"
          onClick={() => setExpanded((value) => !value)}
          disabled={!hasChildren}
          title={hasChildren ? 'Toggle node' : undefined}
        >
          {hasChildren ? (expanded ? 'v' : '>') : ''}
        </button>
        {node.type && <span className="ast-badge">{node.type}</span>}
        <span>{node.label}</span>
      </div>
      {expanded && hasChildren && (
        <div>
          {node.children.map((child) => (
            <TreeNode key={child.id} node={child} />
          ))}
        </div>
      )}
    </div>
  );
}

function cleanLabel(label: string) {
  const nodeKindIndex = nodeKinds
    .map((kind) => label.indexOf(kind))
    .filter((index) => index >= 0)
    .sort((a, b) => a - b)[0];

  if (nodeKindIndex !== undefined) {
    return label.slice(nodeKindIndex).trim();
  }

  return label.replace(/^[\s│├└─\u2500-\u257F?鈹]+/gu, '').trim();
}

function detectNodeType(label: string) {
  return nodeKinds.find((kind) => label.includes(kind)) ?? '';
}
