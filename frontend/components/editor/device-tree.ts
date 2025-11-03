import type { languages } from "monaco-editor";

const dtsKeywords = [
    "/dts-v1/",
    "/include/",
    "true",
    "false",
];

const dtsDirectives = [
    "#include",
    "#address-cells",
    "#size-cells",
    "#compatible",
    "#interrupt-cells",
];

export const dtsLanguage: languages.IMonarchLanguage = {
    defaultToken: "",
    tokenPostfix: ".dts",

    brackets: [
        { open: "{", close: "}", token: "delimiter.curly" },
        { open: "(", close: ")", token: "delimiter.parenthesis" },
        { open: "[", close: "]", token: "delimiter.square" },
    ],

    tokenizer: {
        root: [
            // Line comments
            [/\/\/.*/, "comment"],
            // Block comments
            [/\/\*/, { token: "comment", next: "@comment" }],

            // Directives (#include, #address-cells, etc)
            [
                new RegExp(`\\b(${dtsDirectives.map(d => d.replace(/[-/\\^$*+?.()|[\]{}]/g, "\\$&")).join("|")})\\b`),
                "keyword.directive",
            ],

            // Keywords
            [
                new RegExp(`\\b(${dtsKeywords.map(k => k.replace(/[-/\\^$*+?.()|[\]{}]/g, "\\$&")).join("|")})\\b`),
                "keyword",
            ],

            // Strings
            [/"([^"\\]|\\.)*"/, "string"],

            // Numbers: decimal or hex
            [/\b0x[0-9a-fA-F]+\b/, "number.hex"],
            [/\b\d+\b/, "number"],

            // Labels / nodes: &label
            [/&[a-zA-Z_]\w*/, "variable.parameter"],

            // Node identifiers / property names (fix: numbers at end are part of identifier)
            [/[a-zA-Z_][\w-]*/, "type.identifier"],

            // Property assignments: <...>
            [/<[^>]*>/, "number"],

            // Punctuation
            [/[=;,]/, "delimiter"],
            [/[{}()\[\]]/, "@brackets"],
        ],

        comment: [
            [/[^\*]+/, "comment"],
            [/\*\//, { token: "comment", next: "@pop" }],
            [/[\/*]/, "comment"],
        ],
    },
};
