#!/usr/bin/env node

// ターミナル用 エラーログストリーム CLI
const ErrorGenerators = require('./public/error-generators.js');

const ANSI = {
  reset: "\x1b[0m",
  bold: "\x1b[1m",
  dim: "\x1b[2m",
  italic: "\x1b[3m",
  red: "\x1b[31m",
  green: "\x1b[32m",
  yellow: "\x1b[33m",
  blue: "\x1b[34m",
  magenta: "\x1b[35m",
  cyan: "\x1b[36m",
  white: "\x1b[37m",
  bgRed: "\x1b[41m",
  bgYellow: "\x1b[43m"
};

const args = process.argv.slice(2);
let scenario = 'all';
let intervalMs = 50; // デフォルト20行/秒
let maxLines = Infinity;

for (let i = 0; i < args.length; i++) {
  const arg = args[i];
  if (arg === '--help' || arg === '-h') {
    console.log(`
${ANSI.bold}${ANSI.cyan}エラーログが大量に出てくるだけの暇つぶしのやつ (CLI版)${ANSI.reset}

使用方法:
  node cli.js [options]
  npm run cli -- [options]

オプション:
  --scenario, -s <id>   シナリオ指定:
                        - all       : オールスター・カオス (デフォルト)
                        - cloud     : Web/クラウドインフラ崩壊
                        - kernel    : Linuxカーネルパニック & C/C++
                        - scifi     : SF宇宙船AI暴走
                        - gamedev   : ゲーム開発・物理エンジン崩壊
                        - developer : プログラマー日常の絶望
  --interval, -i <ms>   ログ出力間隔 (ミリ秒, デフォルト: 50)
  --lines, -n <count>   指定行数出力して自動終了
  --help, -h            ヘルプ表示
`);
    process.exit(0);
  } else if (arg === '--scenario' || arg === '-s') {
    scenario = args[++i] || 'all';
  } else if (arg === '--interval' || arg === '-i') {
    intervalMs = parseInt(args[++i] || '50', 10);
  } else if (arg === '--lines' || arg === '-n') {
    maxLines = parseInt(args[++i] || '100', 10);
  }
}

function colorizeLog(raw) {
  // タイムスタンプと本文のカラーハイライト
  let colored = raw;

  // タイムスタンプ
  colored = colored.replace(/^(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}\.\d+)/, `${ANSI.dim}$1${ANSI.reset}`);

  // タグ
  colored = colored.replace(/(\[CRITICAL\]|\[FATAL\]|\[PANIC\]|\[EMERGENCY\])/g, `${ANSI.bgRed}${ANSI.white}${ANSI.bold} $1 ${ANSI.reset}`);
  colored = colored.replace(/(\[ERROR\])/g, `${ANSI.red}${ANSI.bold}$1${ANSI.reset}`);
  colored = colored.replace(/(\[WARN\]|\[ALERT\])/g, `${ANSI.yellow}${ANSI.bold}$1${ANSI.reset}`);
  colored = colored.replace(/(\[INFO\]|\[SYSTEM\])/g, `${ANSI.cyan}$1${ANSI.reset}`);

  // アドレス / IP
  colored = colored.replace(/(0x[0-9a-fA-F]+)/g, `${ANSI.magenta}$1${ANSI.reset}`);
  colored = colored.replace(/(\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b)/g, `${ANSI.cyan}$1${ANSI.reset}`);

  return colored;
}

let lineCount = 0;
const startTime = Date.now();

console.log(`${ANSI.bold}${ANSI.red}🚨 [SYSTEM FAILURE SIMULATOR] ログストリーム開始... (Ctrl+C で脱出)${ANSI.reset}`);
console.log(`${ANSI.dim}シナリオ: ${scenario} | 間隔: ${intervalMs}ms${ANSI.reset}\n`);

const timer = setInterval(() => {
  const line = ErrorGenerators.generateLogLine(scenario);
  console.log(colorizeLog(line));
  lineCount++;

  if (lineCount >= maxLines) {
    shutdown();
  }
}, intervalMs);

function shutdown() {
  clearInterval(timer);
  const duration = ((Date.now() - startTime) / 1000).toFixed(1);
  console.log(`\n${ANSI.bold}${ANSI.green}✨ 暇はつぶせましたか？${ANSI.reset}`);
  console.log(`${ANSI.dim}総出力エラー数: ${lineCount.toLocaleString()} 行 | 稼働時間: ${duration} 秒${ANSI.reset}\n`);
  process.exit(0);
}

process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);
