# DTS Studio - Frontend (Tauri + React + Tailwind + shadcn/ui)

This is the desktop frontend built with Tauri v2, React + Vite + TypeScript, Tailwind CSS, and shadcn/ui.

## Prerequisites

- Node.js 18+
- Rust toolchain (for Tauri)
- On Windows, ensure required Tauri dependencies are installed: https://tauri.app/start/prerequisites/

## Install

```pwsh
# from repo root or this folder
cd frontend
npm install
```

## Develop (Tauri window)

```pwsh
npm run tauri dev
```

## Build production bundle

```pwsh
# TypeScript + Vite build only (no Rust)
npm run build

# Full desktop build (requires Rust toolchain)
npm run tauri build
```

## Tech notes

- Tailwind CSS configured in `tailwind.config.js` and `postcss.config.js`.
- shadcn/ui initialized via `components.json`. Add components with:

```pwsh
npx shadcn@latest add button
```

- Import aliases:
	- `@` -> `src`
	- Example: `import { Button } from "@/components/ui/button"`

## File map

- `src/index.css` — Tailwind layers and CSS variables (shadcn theme)
- `src/components/ui/` — shadcn/ui components (e.g., `button.tsx`)
- `src/lib/utils.ts` — shadcn helpers (cn)
- `vite.config.ts` — Vite alias configured for `@`
- `src-tauri/` — Tauri Rust sources (command `greet` available)

## Troubleshooting

- If Vite can't resolve `@/...`, make sure `vite.config.ts` has the alias and `tsconfig.json` has the path mapping.
- If types fail for Node imports in Vite config, ensure `@types/node` is installed.
