// フロントエンド コントローラー
document.addEventListener('DOMContentLoaded', () => {
  const { generateLogLine, generateEmergencyFixSequence, generateSelfDestructSequence } = window.ErrorGenerators;

  // DOM要素
  const logOutput = document.getElementById('logOutput');
  const terminalContainer = document.getElementById('terminalContainer');
  const scenarioSelect = document.getElementById('scenarioSelect');
  const speedSlider = document.getElementById('speedSlider');
  const speedDisplay = document.getElementById('speedDisplay');
  const btnPause = document.getElementById('btnPause');
  const btnStep = document.getElementById('btnStep');
  const btnEmergencyFix = document.getElementById('btnEmergencyFix');
  const btnHackerMode = document.getElementById('btnHackerMode');
  const btnSelfDestruct = document.getElementById('btnSelfDestruct');
  const btnSound = document.getElementById('btnSound');
  const btnCrt = document.getElementById('btnCrt');
  const btnClear = document.getElementById('btnClear');
  const btnCopy = document.getElementById('btnCopy');
  const btnDownload = document.getElementById('btnDownload');
  const btnFullscreen = document.getElementById('btnFullscreen');
  const scrollBottomBtn = document.getElementById('scrollBottomBtn');
  const hackerBar = document.getElementById('hackerBar');
  const sevBadge = document.getElementById('sevBadge');

  // メトリクス要素
  const counterTotal = document.getElementById('counterTotal');
  const counterRate = document.getElementById('counterRate');
  const counterMem = document.getElementById('counterMem');
  const counterUptime = document.getElementById('counterUptime');

  // モーダル要素
  const modalBackdrop = document.getElementById('modalBackdrop');
  const modalClose = document.getElementById('modalClose');
  const modalConfirmBtn = document.getElementById('modalConfirmBtn');
  const modalLogContent = document.getElementById('modalLogContent');
  const modalExcuseText = document.getElementById('modalExcuseText');
  const btnRerollExcuse = document.getElementById('btnRerollExcuse');

  // 状態管理
  let isRunning = true;
  let isHackerMode = false;
  let isSoundEnabled = false;
  let isAutoScroll = true;
  let linesPerSec = parseInt(speedSlider.value, 10);
  let totalLogs = 0;
  let recentLogsCount = 0;
  let heapLeakBase = 98.4;
  const startTime = Date.now();
  const MAX_DOM_LINES = 2500; // パフォーマンス維持のための上限保持行数
  let rawLogsHistory = [];

  // Web Audio API
  let audioCtx = null;
  function getAudioContext() {
    if (!audioCtx) {
      audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    }
    if (audioCtx.state === 'suspended') {
      audioCtx.resume();
    }
    return audioCtx;
  }

  function playBeep(freq = 440, duration = 0.05, type = 'sine') {
    if (!isSoundEnabled) return;
    try {
      const ctx = getAudioContext();
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.type = type;
      osc.frequency.setValueAtTime(freq, ctx.currentTime);
      gain.gain.setValueAtTime(0.08, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + duration);
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.start();
      osc.stop(ctx.currentTime + duration);
    } catch (e) {
      // Audio not supported or blocked
    }
  }

  function playAlertAlarm() {
    if (!isSoundEnabled) return;
    try {
      const ctx = getAudioContext();
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.type = 'sawtooth';
      osc.frequency.setValueAtTime(800, ctx.currentTime);
      osc.frequency.linearRampToValueAtTime(300, ctx.currentTime + 0.3);
      gain.gain.setValueAtTime(0.12, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.3);
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.start();
      osc.stop(ctx.currentTime + 0.3);
    } catch (e) {}
  }

  function playExplosion() {
    if (!isSoundEnabled) return;
    try {
      const ctx = getAudioContext();
      const bufferSize = ctx.sampleRate * 0.4;
      const buffer = ctx.createBuffer(1, bufferSize, ctx.sampleRate);
      const data = buffer.getChannelData(0);
      for (let i = 0; i < bufferSize; i++) {
        data[i] = Math.random() * 2 - 1;
      }
      const noise = ctx.createBufferSource();
      noise.buffer = buffer;
      const filter = ctx.createBiquadFilter();
      filter.type = 'lowpass';
      filter.frequency.setValueAtTime(800, ctx.currentTime);
      filter.frequency.linearRampToValueAtTime(50, ctx.currentTime + 0.4);
      const gain = ctx.createGain();
      gain.gain.setValueAtTime(0.2, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.4);
      noise.connect(filter);
      filter.connect(gain);
      gain.connect(ctx.destination);
      noise.start();
    } catch (e) {}
  }

  // ログ行のHTMLフォーマット
  function formatLogToHtml(rawLog) {
    // タイムスタンプと本文の分離
    // 例: 2026-10-12 12:34:56.789123 [CRITICAL] ...
    const match = rawLog.match(/^(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}\.\d+)\s+(\[.+?\])?(.*)$/s);
    if (!match) {
      return `<div class="log-row"><span class="log-text">${escapeHtml(rawLog)}</span></div>`;
    }

    const time = match[1];
    const tag = match[2] || '';
    const rest = match[3] || '';

    let tagClass = 'tag-error';
    if (/CRITICAL|FATAL|PANIC|EMERGENCY/i.test(tag)) tagClass = 'tag-critical';
    else if (/WARN|ALERT/i.test(tag)) tagClass = 'tag-warn';
    else if (/INFO|SYSTEM/i.test(tag)) tagClass = 'tag-info';

    // 本文内のハイライト（アドレス、IP、数値など）
    const highlightedRest = escapeHtml(rest)
      .replace(/(0x[0-9a-fA-F]+)/g, '<span class="text-purple">$1</span>')
      .replace(/(\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b)/g, '<span class="text-cyan">$1</span>')
      .replace(/(T-minus \d+)/g, '<span class="text-yellow font-bold">$1</span>');

    return `<div class="log-row" data-raw="${escapeAttr(rawLog)}">
      <span class="log-time">${time}</span>
      ${tag ? `<span class="log-tag ${tagClass}">${escapeHtml(tag)}</span>` : ''}
      <span class="log-text">${highlightedRest}</span>
    </div>`;
  }

  function escapeHtml(str) {
    return str
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#039;');
  }

  function escapeAttr(str) {
    return str.replace(/"/g, '&quot;');
  }

  // ログの追加
  function appendLogs(logs, isCascade = false) {
    if (!logs || logs.length === 0) return;

    const fragment = document.createDocumentFragment();
    const tempDiv = document.createElement('div');

    const htmlString = logs.map(formatLogToHtml).join('');
    tempDiv.innerHTML = htmlString;

    while (tempDiv.firstChild) {
      const child = tempDiv.firstChild;
      if (isCascade) {
        child.classList.add('highlight-cascade');
      }
      fragment.appendChild(child);
    }

    logOutput.appendChild(fragment);
    totalLogs += logs.length;
    recentLogsCount += logs.length;

    // 生ログ配列に保存（保存・コピー用）
    for (let i = 0; i < logs.length; i++) {
      rawLogsHistory.push(logs[i]);
    }
    if (rawLogsHistory.length > 5000) {
      rawLogsHistory = rawLogsHistory.slice(-4000);
    }

    // DOM行数間引き（溢れたら上から削除）
    while (logOutput.children.length > MAX_DOM_LINES) {
      logOutput.removeChild(logOutput.firstChild);
    }

    // 自動スクロール
    if (isAutoScroll) {
      terminalContainer.scrollTop = terminalContainer.scrollHeight;
    }

    // 音声
    if (isSoundEnabled) {
      if (isCascade) {
        playAlertAlarm();
      } else {
        const freq = 400 + (Math.random() * 300);
        playBeep(freq, 0.03, 'sine');
      }
    }
  }

  // スクロール監視
  terminalContainer.addEventListener('scroll', () => {
    const isAtBottom = (terminalContainer.scrollHeight - terminalContainer.scrollTop - terminalContainer.clientHeight) < 40;
    isAutoScroll = isAtBottom;
    scrollBottomBtn.style.display = isAtBottom ? 'none' : 'block';
  });

  scrollBottomBtn.addEventListener('click', () => {
    isAutoScroll = true;
    terminalContainer.scrollTop = terminalContainer.scrollHeight;
    scrollBottomBtn.style.display = 'none';
  });

  // スピードスライダー
  speedSlider.addEventListener('input', (e) => {
    linesPerSec = parseInt(e.target.value, 10);
    speedDisplay.textContent = `${linesPerSec} 行/秒`;
  });

  // 一時停止 / 再開
  btnPause.addEventListener('click', () => {
    isRunning = !isRunning;
    if (isRunning) {
      btnPause.innerHTML = '⏸️ 一時停止';
      btnPause.classList.remove('btn-secondary');
      btnPause.classList.add('btn-primary');
    } else {
      btnPause.innerHTML = '▶️ 再開';
      btnPause.classList.remove('btn-primary');
      btnPause.classList.add('btn-secondary');
    }
  });

  // +1行手動
  btnStep.addEventListener('click', () => {
    const scenario = scenarioSelect.value;
    appendLogs([generateLogLine(scenario)]);
  });

  // 🚨 緊急復旧試行（大惨事ボタン）
  btnEmergencyFix.addEventListener('click', () => {
    btnEmergencyFix.disabled = true;
    btnEmergencyFix.textContent = '⏳ 復旧パッチ適用中...';

    // 画面激震エフェクト
    document.body.classList.add('glitch-active');
    setTimeout(() => document.body.classList.remove('glitch-active'), 400);

    const fixSeq = generateEmergencyFixSequence();
    let index = 0;

    const fixInterval = setInterval(() => {
      if (index < fixSeq.length) {
        appendLogs([fixSeq[index]], index >= 7);
        index++;
      } else {
        clearInterval(fixInterval);
        btnEmergencyFix.disabled = false;
        btnEmergencyFix.textContent = '🚨 緊急復旧試行';
        
        // 速度を一時的に超高速化
        const origSpeed = linesPerSec;
        linesPerSec = Math.min(100, origSpeed * 3);
        speedSlider.value = linesPerSec;
        speedDisplay.textContent = `${linesPerSec} 行/秒 (暴走中!)`;
        sevBadge.textContent = 'DEFCON 0 : APOCALYPTIC CASCADE';

        setTimeout(() => {
          linesPerSec = origSpeed;
          speedSlider.value = linesPerSec;
          speedDisplay.textContent = `${linesPerSec} 行/秒`;
          sevBadge.textContent = 'DEFCON 1 : TOTAL FAILURE';
        }, 5000);
      }
    }, 180);
  });

  // 💥 自爆シーケンス
  btnSelfDestruct.addEventListener('click', () => {
    if (!confirm('⚠️ 本当に自爆シーケンスを起動しますか？\n（暇つぶしが最高潮に達します）')) {
      return;
    }

    const destructSeq = generateSelfDestructSequence();
    let index = 0;
    playAlertAlarm();

    const interval = setInterval(() => {
      if (index < destructSeq.length) {
        appendLogs([destructSeq[index]], true);
        document.body.classList.add('glitch-active');
        setTimeout(() => document.body.classList.remove('glitch-active'), 250);
        index++;
      } else {
        clearInterval(interval);
        playExplosion();
      }
    }, 350);
  });

  // ⌨️ ハッカー連打モード
  btnHackerMode.addEventListener('click', () => {
    isHackerMode = !isHackerMode;
    if (isHackerMode) {
      btnHackerMode.classList.add('active');
      hackerBar.style.display = 'block';
    } else {
      btnHackerMode.classList.remove('active');
      hackerBar.style.display = 'none';
    }
  });

  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && isHackerMode) {
      isHackerMode = false;
      btnHackerMode.classList.remove('active');
      hackerBar.style.display = 'none';
      return;
    }

    if (isHackerMode) {
      e.preventDefault();
      const count = Math.floor(Math.random() * 3) + 1;
      const logs = [];
      const scenario = scenarioSelect.value;
      for (let i = 0; i < count; i++) {
        logs.push(generateLogLine(scenario));
      }
      appendLogs(logs);
      playBeep(250 + Math.random() * 400, 0.02, 'square');
    }
  });

  // 🔊 サウンド切替
  btnSound.addEventListener('click', () => {
    isSoundEnabled = !isSoundEnabled;
    if (isSoundEnabled) {
      getAudioContext();
      btnSound.textContent = '🔊 サウンド: ON';
      btnSound.style.color = 'var(--green)';
      playBeep(600, 0.1);
    } else {
      btnSound.textContent = '🔇 サウンド: OFF';
      btnSound.style.color = '';
    }
  });

  // 📺 CRT切替
  btnCrt.addEventListener('click', () => {
    document.body.classList.toggle('crt-enabled');
    const isCrt = document.body.classList.contains('crt-enabled');
    btnCrt.textContent = isCrt ? '📺 CRT: ON' : '📺 CRT: OFF';
  });

  // 🧹 ログクリア
  btnClear.addEventListener('click', () => {
    logOutput.innerHTML = '';
    rawLogsHistory = [];
  });

  // 📋 コピー
  btnCopy.addEventListener('click', () => {
    if (rawLogsHistory.length === 0) {
      alert('コピーするログがありません。');
      return;
    }
    const text = rawLogsHistory.slice(-200).join('\n');
    navigator.clipboard.writeText(text).then(() => {
      const orig = btnCopy.textContent;
      btnCopy.textContent = '✅ コピー完了!';
      setTimeout(() => btnCopy.textContent = orig, 1500);
    }).catch(() => {
      alert('クリップボードへのアクセスが許可されていません');
    });
  });

  // 💾 保存
  btnDownload.addEventListener('click', () => {
    if (rawLogsHistory.length === 0) {
      alert('保存するログがありません。');
      return;
    }
    const blob = new Blob([rawLogsHistory.join('\n')], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `error_log_${new Date().toISOString().replace(/[:.]/g, '-')}.txt`;
    a.click();
    URL.revokeObjectURL(url);
  });

  // 🖥️ 全画面表示
  btnFullscreen.addEventListener('click', () => {
    if (!document.fullscreenElement) {
      document.documentElement.requestFullscreen().catch(() => {});
    } else {
      if (document.exitFullscreen) {
        document.exitFullscreen();
      }
    }
  });

  // 言い訳ジェネレーター
  const EXCUSES = [
    "私のローカル環境（MacBook Pro）では正常に動いていました。",
    "これはバグではなく、将来のバージョンに向けた実験的仕様です。",
    "前任者が書いたコードなので私は何も触っていません。",
    "おそらくブラウザのキャッシュかDNSの浸透待ちが原因です。",
    "昨日の夜までは完璧に動いていました。天体の配置が変わったのかもしれません。",
    "仕様書に「エラーを出してはいけない」とは一言も書かれていませんでした。",
    "AIが生成したコードなので、私は被害者の一人です。",
    "Dockerを再起動すれば80%の確率で自然治癒します。",
    "これは想定内のフェイルセーフです（冷や汗を拭いながら）。",
    "リリース日が金曜日の夕方だったのがすべての元凶です。"
  ];

  function getRandomExcuse() {
    return EXCUSES[Math.floor(Math.random() * EXCUSES.length)];
  }

  // ログ行クリックで詳細モーダル表示
  logOutput.addEventListener('click', (e) => {
    const row = e.target.closest('.log-row');
    if (!row) return;

    const raw = row.getAttribute('data-raw') || row.innerText;
    modalLogContent.textContent = raw;
    modalExcuseText.textContent = getRandomExcuse();
    modalBackdrop.classList.add('show');
  });

  modalClose.addEventListener('click', () => modalBackdrop.classList.remove('show'));
  modalConfirmBtn.addEventListener('click', () => modalBackdrop.classList.remove('show'));
  btnRerollExcuse.addEventListener('click', () => {
    modalExcuseText.textContent = getRandomExcuse();
  });
  modalBackdrop.addEventListener('click', (e) => {
    if (e.target === modalBackdrop) modalBackdrop.classList.remove('show');
  });

  // ログ生成ループ
  let accumulator = 0;
  let lastTimestamp = performance.now();

  function streamLoop(currentTimestamp) {
    const delta = (currentTimestamp - lastTimestamp) / 1000;
    lastTimestamp = currentTimestamp;

    if (isRunning && !isHackerMode) {
      accumulator += delta * linesPerSec;
      const linesToProduce = Math.floor(accumulator);
      if (linesToProduce > 0) {
        accumulator -= linesToProduce;
        const scenario = scenarioSelect.value;
        const batch = [];
        // 1フレームあたりの過大描画を制限 (最大30行/フレーム)
        const count = Math.min(linesToProduce, 30);
        for (let i = 0; i < count; i++) {
          batch.push(generateLogLine(scenario));
        }
        appendLogs(batch);
      }
    }

    requestAnimationFrame(streamLoop);
  }

  requestAnimationFrame(streamLoop);

  // メトリクス更新タイマー（1秒毎）
  setInterval(() => {
    // 総エラー数
    counterTotal.textContent = totalLogs.toLocaleString();

    // 秒間エラーレート
    counterRate.textContent = `${recentLogsCount} /s`;
    recentLogsCount = 0;

    // メモリリーク率 (演出用)
    heapLeakBase = Math.min(99.9, heapLeakBase + (Math.random() * 0.05 - 0.02));
    counterMem.textContent = `${heapLeakBase.toFixed(1)}%`;

    // アップタイム
    const elapsed = Math.floor((Date.now() - startTime) / 1000);
    const hh = String(Math.floor(elapsed / 3600)).padStart(2, '0');
    const mm = String(Math.floor((elapsed % 3600) / 60)).padStart(2, '0');
    const ss = String(elapsed % 60).padStart(2, '0');
    counterUptime.textContent = `${hh}:${mm}:${ss}`;
  }, 1000);

  // 初期起動ログ
  appendLogs([
    `[SYSTEM] Initialization started at ${new Date().toISOString()}...`,
    `[INFO] Loading core modules: 1,489 modules loaded (0 verified)`,
    `[WARN] Security policy bypass enabled: running in UNRESTRICTED_CHAOS_MODE`,
    `[INFO] Welcome to Error Log Stream. Prepare for catastrophic enjoyment.`,
    generateLogLine('all'),
    generateLogLine('all')
  ]);
});
