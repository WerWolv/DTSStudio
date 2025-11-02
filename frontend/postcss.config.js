import tailwindcss from "tailwindcss";
import autoprefixer from "autoprefixer";
import { fileURLToPath } from "node:url";
import { dirname, resolve } from "node:path";

const __dirname = dirname(fileURLToPath(import.meta.url));

export default {
  plugins: [
    // Point Tailwind explicitly to this folder's config to avoid CWD issues
    tailwindcss({ config: resolve(__dirname, "tailwind.config.js") }),
    autoprefixer(),
  ],
};
