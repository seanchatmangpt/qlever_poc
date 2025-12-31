const path = require("path");
const webpack = require("webpack");

module.exports = [
  {
    mode: "production",
    entry: "./src/browser.ts",
    output: {
      filename: "browser.js",
      path: path.resolve(__dirname, "dist"),
      library: "qleverWasm",
      libraryTarget: "umd",
    },
    module: {
      rules: [
        {
          test: /\.tsx?$/,
          use: "ts-loader",
          exclude: /node_modules/,
        },
        {
          test: /\.wasm$/,
          type: "webassembly/async",
        },
      ],
    },
    resolve: {
      extensions: [".ts", ".tsx", ".js"],
      alias: {
        "@": path.resolve(__dirname, "src/"),
      },
    },
    experiments: {
      asyncWebAssembly: true,
    },
  },
  {
    mode: "production",
    entry: "./src/node.ts",
    output: {
      filename: "node.js",
      path: path.resolve(__dirname, "dist"),
      libraryTarget: "umd",
    },
    target: "node",
    module: {
      rules: [
        {
          test: /\.tsx?$/,
          use: "ts-loader",
          exclude: /node_modules/,
        },
        {
          test: /\.wasm$/,
          type: "webassembly/async",
        },
      ],
    },
    resolve: {
      extensions: [".ts", ".tsx", ".js"],
      alias: {
        "@": path.resolve(__dirname, "src/"),
      },
    },
    experiments: {
      asyncWebAssembly: true,
    },
  },
];
