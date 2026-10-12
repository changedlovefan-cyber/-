const http = require('http');
const fs = require('fs');
const path = require('path');
const ErrorGenerators = require('./public/error-generators.js');

const PORT = process.env.PORT || 3000;
const HOST = '0.0.0.0';
const PUBLIC_DIR = path.join(__dirname, 'public');

const MIME_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.ico': 'image/x-icon',
  '.txt': 'text/plain; charset=utf-8'
};

const server = http.createServer((req, res) => {
  // CORSヘッダー
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, OPTIONS');

  if (req.method === 'OPTIONS') {
    res.writeHead(204);
    res.end();
    return;
  }

  const parsedUrl = new URL(req.url, `http://${req.headers.host || 'localhost'}`);
  let pathname = parsedUrl.pathname;

  // 1. API: シナリオ一覧
  if (pathname === '/api/scenarios') {
    res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
    const list = Object.values(ErrorGenerators.SCENARIOS).map(s => ({
      id: s.id,
      name: s.name,
      desc: s.desc
    }));
    res.end(JSON.stringify({ scenarios: list }, null, 2));
    return;
  }

  // 2. API: 単一またはバッチログ生成
  if (pathname === '/api/log') {
    const scenario = parsedUrl.searchParams.get('scenario') || 'all';
    const count = Math.min(parseInt(parsedUrl.searchParams.get('count') || '1', 10), 100);
    const logs = [];
    for (let i = 0; i < count; i++) {
      logs.push(ErrorGenerators.generateLogLine(scenario));
    }
    res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
    res.end(JSON.stringify({ logs }, null, 2));
    return;
  }

  // 3. API: SSE (Server-Sent Events) ログストリーム
  if (pathname === '/api/stream') {
    const scenario = parsedUrl.searchParams.get('scenario') || 'all';
    const intervalMs = Math.max(20, parseInt(parsedUrl.searchParams.get('interval') || '100', 10));

    res.writeHead(200, {
      'Content-Type': 'text/event-stream',
      'Cache-Control': 'no-cache',
      'Connection': 'keep-alive'
    });

    res.write(`data: ${JSON.stringify({ log: `[SYSTEM] SSE stream connected for scenario '${scenario}'` })}\n\n`);

    const streamInterval = setInterval(() => {
      const logLine = ErrorGenerators.generateLogLine(scenario);
      res.write(`data: ${JSON.stringify({ log: logLine })}\n\n`);
    }, intervalMs);

    req.on('close', () => {
      clearInterval(streamInterval);
    });
    return;
  }

  // 4. 静的ファイル配信
  if (pathname === '/' || pathname === '') {
    pathname = '/index.html';
  }

  // ディレクトリトラバーサル対策
  const safePath = path.normalize(pathname).replace(/^(\.\.[\/\\])+/, '');
  const filePath = path.join(PUBLIC_DIR, safePath);

  if (!filePath.startsWith(PUBLIC_DIR)) {
    res.writeHead(403, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end('403 Forbidden');
    return;
  }

  fs.stat(filePath, (err, stats) => {
    if (err || !stats.isFile()) {
      res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
      res.end('404 Not Found');
      return;
    }

    const ext = path.extname(filePath).toLowerCase();
    const contentType = MIME_TYPES[ext] || 'application/octet-stream';

    res.writeHead(200, { 'Content-Type': contentType });
    const stream = fs.createReadStream(filePath);
    stream.pipe(res);
  });
});

server.listen(PORT, HOST, () => {
  console.log(`=================================================`);
  console.log(`💥 ERROR LOG STREAM サーバー起動完了!`);
  console.log(`🚀 ブラウザでアクセス: http://${HOST}:${PORT}`);
  console.log(`=================================================`);
});
