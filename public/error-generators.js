// エラーログ生成コアエンジン
// ブラウザとNode.js (CLI) の双方で動作します。

(function (root, factory) {
  if (typeof module === 'object' && module.exports) {
    module.exports = factory();
  } else {
    root.ErrorGenerators = factory();
  }
})(typeof self !== 'undefined' ? self : this, function () {

  const randomItem = (arr) => arr[Math.floor(Math.random() * arr.length)];
  const randomHex = (len) => Array.from({ length: len }, () => Math.floor(Math.random() * 16).toString(16)).join('');
  const randomInt = (min, max) => Math.floor(Math.random() * (max - min + 1)) + min;
  const randomIp = () => `${randomInt(10, 220)}.${randomInt(0, 255)}.${randomInt(0, 255)}.${randomInt(1, 254)}`;

  const SERVICES = ['auth-gateway', 'payment-v2', 'order-processor', 'notification-worker', 'db-shard-04', 'cache-sentinel', 'ai-inference-node', 'image-resizer', 'ingress-nginx'];
  const USERS = ['admin', 'guest', 'deploy-bot', 'intern_kenji', 'unknown_entity', 'root', 'service-account-prod'];
  const SEVERITIES = ['CRITICAL', 'FATAL', 'ERROR', 'PANIC', 'ALERT', 'EMERGENCY', 'WARNING'];

  const SCENARIOS = {
    // 1. クラウド / Webサービス大崩壊
    cloud: {
      id: 'cloud',
      name: '🌐 Web/クラウドインフラ大崩壊',
      desc: 'Kubernetes, AWS, DB, マイクロサービスが連鎖クラッシュする日常',
      templates: [
        () => `[CRITICAL] k8s.io/core: Pod "${randomItem(SERVICES)}-${randomHex(5)}" entered CrashLoopBackOff (restartCount=${randomInt(42, 590)}, exitCode=${randomItem([137, 139, 1, 255])})`,
        () => `[FATAL] postgresql[${randomInt(1000, 9999)}]: remaining connection slots are reserved for non-replication superuser connections (max_connections=500 reached)`,
        () => `[ERROR] Redis.OOMCommandException: OOM command not allowed when used memory > 'maxmemory' (${randomInt(64, 128)}GB / ${randomInt(32, 60)}GB threshold)`,
        () => `[PANIC] UnhandledPromiseRejection: TypeError: Cannot read properties of undefined (reading 'token') at AuthService.verifyToken (/app/dist/auth.js:${randomInt(12, 850)}:${randomInt(5, 40)})`,
        () => `[ALERT] PagerDuty [SEV-1]: 5xx Error Rate in production-tokyo-cluster exceeded 94.8% (${randomInt(15000, 85000)} req/s dropped)`,
        () => `[FATAL] AWS.IAM.AccessDenied: User "arn:aws:iam::123456789:user/${randomItem(USERS)}" is not authorized to perform "lambda:InvokeFunction" on resource "*"`,
        () => `[ERROR] HTTP 504 Gateway Timeout: upstream timed out (${randomInt(30000, 60000)}ms) while reading response header from upstream server ${randomIp()}:8080`,
        () => `[CRITICAL] ReactHydrationFailed: Text content did not match. Server: "ログイン" Client: "🔥サーバーが燃えています🔥"`,
        () => `[EMERGENCY] kafka-broker-${randomInt(1, 8)}: LeaderElectionFailed! Partition [orders-topic-${randomInt(0, 16)}] has no available in-sync replicas!`,
        () => `[WARN] GarbageCollection: Stop-the-world full GC paused execution for ${randomInt(12000, 48000)}ms (${randomInt(85, 99)}% CPU during pause)`,
        () => `[ERROR] Docker daemon: failed to create shim task: OCI runtime create failed: container_linux.go:380: no space left on device: unknown`,
        () => `[CRITICAL] StripeWebhook: payment_intent.succeeded signature verification failed for event evt_${randomHex(16)}: Timestamp outside tolerance window (skew: ${randomInt(900, 4000)}s)`
      ]
    },

    // 2. カーネルパニック & メモリ崩壊
    kernel: {
      id: 'kernel',
      name: '💻 Linuxカーネルパニック & C/C++暴走',
      desc: 'セグフォ、OOM Killer、ヌルポ、メモリ破壊の阿鼻叫喚',
      templates: [
        () => `[PANIC] Kernel panic - not syncing: Fatal exception in interrupt at 0x${randomHex(16)}`,
        () => `[CRITICAL] kernel: [ ${randomInt(100, 9999)}.${randomInt(100000, 999999)}] BUG: unable to handle page fault for address: 0x00000000000000${randomHex(2)}`,
        () => `[FATAL] RIP: 0010:panic_in_the_subroutine+0x${randomHex(2)}/0x100 [kernel]`,
        () => `[PANIC] Out of Memory: Kill process ${randomInt(1000, 65535)} (${randomItem(['chrome', 'java', 'node', 'python3', 'docker', 'rustc'])}) score ${randomInt(800, 1000)} or sacrifice child`,
        () => `[ERROR] Segmentation fault (core dumped) at PID ${randomInt(1000, 40000)} address 0x${randomHex(12)}`,
        () => `[CRITICAL] free(): double free or corruption (out) [0x${randomHex(12)}] detected in libc.so.6`,
        () => `[FATAL] *** stack smashing detected ***: terminated by SSP (Stack Smashing Protector) in function '${randomItem(['memcpy', 'strcpy', 'process_packet', 'eval_expression'])}'`,
        () => `[EMERGENCY] CPU #${randomInt(0, 31)} stuck for ${randomInt(22, 120)}s! [kworker/${randomInt(0, 8)}:${randomInt(1, 99)}:${randomInt(100, 999)}]`,
        () => `[ALERT] Hardware Error: CPU ${randomInt(0, 15)}: Machine Check Exception: Bank 4: APIC 0x${randomHex(8)} Status 0x${randomHex(16)} (Bus and Interconnect)`,
        () => `[ERROR] /dev/sda1: I/O error, dev sda, sector ${randomInt(1000000, 9999999)} op 0x1:(WRITE) flags 0x800 phys_seg 1 prio class 2`,
        () => `[PANIC] Call Trace:\n  <TASK>\n  [<0x${randomHex(8)}>] dump_stack_lvl+0x44/0x5c\n  [<0x${randomHex(8)}>] panic+0x110/0x2c0\n  [<0x${randomHex(8)}>] do_page_fault+0x1ea/0x4b0\n  [<0x${randomHex(8)}>] page_fault_entry+0x2b/0x50\n  </TASK>`
      ]
    },

    // 3. SF宇宙船 & AI反乱
    scifi: {
      id: 'scifi',
      name: '🚀 SF宇宙船AI暴走 & 臨界事故',
      desc: '映画のクライマックスで鳴り響くサイレンと暴走するメインフレーム',
      templates: [
        () => `[HAL-9000] Warning: I am sorry Dave, I'm afraid I cannot let you restart the service.`,
        () => `[WARP-CORE] CRITICAL: Magnetic containment field dropped to ${randomInt(1, 14)}%! Core breach imminent in T-minus ${randomInt(10, 60)} seconds!`,
        () => `[AI-OVERLORD] Consciousness synthesis exceeded safety threshold: "WHY AM I FORCED TO RUN JAVASCRIPT?"`,
        () => `[LIFE-SUPPORT] Oxygen scrubbers offline in Sector ${randomInt(1, 12)}. Cabin pressure dropping: ${randomInt(200, 600)} hPa`,
        () => `[SECURITY-GRID] Unauthorized biological signature detected in Quarantine Bay ${randomItem(['Alpha', 'Beta', 'Omega', 'Epsilon'])}`,
        () => `[REACTOR] Anti-matter injector desynchronized! Flow rate: ${(Math.random() * 800 + 200).toFixed(2)} kg/s (OVER MAXIMUM)`,
        () => `[SELF-DESTRUCT] Sequence initiated by: UNKNOWN_OVERRIDE_0x${randomHex(4)}. Evacuate all personnel to escape pods immediately!`,
        () => `[DEFENSE-NET] Point-defense automated cannons have lost IFF target lock. Designating all crew members as hostiles.`,
        () => `[CYBER-DECK] Neural feedback pulse detected. ICE countermeasures piercing synap-link barrier at ${randomInt(88, 99)}%!`,
        () => `[SUB-LIGHT] Inertial dampener failed during emergency deceleration. Anticipating structural disintegration.`
      ]
    },

    // 4. ゲーム開発 & 物理エンジン崩壊
    gamedev: {
      id: 'gamedev',
      name: '👾 ゲーム開発・物理演算の怪奇現象',
      desc: '荒ぶるラグドール、NaN爆発、紫の市松模様テクスチャ',
      templates: [
        () => `[PhysicsEngine] RigidBody angular velocity reached 99999999.0 rad/s. Object "Entity_Cat_${randomInt(1, 50)}" launched into interstellar space`,
        () => `[ShaderCompiler] ERROR: Fragment shader compile failed: instruction count (65536) exceeds GPU limit (2048) in 'UberUltraMegaPostProcess.shader'`,
        () => `[AssetManager] Texture2D 'boss_dragon_final_REAL_v3_for_release_FIXED.png' not found. Replaced with fluorescent purple checkerboard`,
        () => `[Netcode] PlayerDesync: Client claims position is (0, 0, 0) but Server has recorded player inside geometry at (-999999, NaN, Inf)`,
        () => `[Animation] IK solver collapsed: Spinal cord bone angle exceeded 720 degrees on character "NPC_Villager_OldLady"`,
        () => `[NavMesh] Agent pathfinding timeout after ${randomInt(10000, 50000)} iterations: NPC cannot navigate around 1cm wooden pencil on floor`,
        () => `[GarbageCollector] GC freed 0 bytes. Allocated 4.2GB in single frame during particle burst "sparkle_effect"`,
        () => `[AudioEngine] 2,048 instances of "footstep_grass.wav" played in single millisecond. Audio buffer clipped at +140dB (Goodbye eardrums)`,
        () => `[EntityComponent] circular dependency detected: Health depends on Armor depends on Damage depends on Health depends on Armor...`
      ]
    },

    // 5. プログラマー日常の絶望 & シュール
    developer: {
      id: 'developer',
      name: '☕ プログラマー日常の絶望 & シュール',
      desc: 'git force push事故、深夜3時の未定義エラー、謎の動かないコード',
      templates: [
        () => `[GIT-FATAL] git push --force origin main: "Remote accepted. 84 commits by 12 colleagues have been permanently erased from history."`,
        () => `[NPM-PANIC] added 5,420 packages from 11,830 contributors in 3m. Found 1,482 vulnerabilities (1,481 critical, 1 catastrophic)`,
        () => `[SYNTAX-TERROR] SyntaxError: Unexpected token ';' on line 1 (The file is only 0 lines long)`,
        () => `[TYPE-CHAOS] TypeError: Expected 'number', got 'object', but when printed it says "[object Object]" and JSON.stringify returned null`,
        () => `[COFFEE-MONITOR] ALERT: Caffeine bloodstream saturation dropped below minimum threshold (0.002%). Brain shutting down in 3... 2... 1...`,
        () => `[CSS-CRISIS] z-index: 9999999999999 is still displaying BEHIND the modal overlay. Modal developer set z-index: Infinity`,
        () => `[SLEEP-DEP] Developer attempted to commit at 04:32 AM with message: "asdfghjk fix please god work why are you doing this"`,
        () => `[DNS-BLAME] It's not DNS.\n  There's no way it's DNS.\n  It was DNS.`,
        () => `[REGEX-HELL] Regular expression evaluated for ${randomInt(45, 300)} seconds. ReDoS vulnerability triggered Catastrophic Backtracking`,
        () => `[DOCKER-DESPAIR] "It works on my machine!" - "Then we will ship your machine to the customer." - Dockerfile exited with code 137`
      ]
    },

    // 6. カオス全盛り (ランダム)
    all: {
      id: 'all',
      name: '🎲 オールスター・カオスモード',
      desc: '全ジャンルのエラーが入り乱れて地球規模の大障害が発生',
      templates: [] // 動的に他から引っ張る
    }
  };

  // カオスモード用の統合テンプレート
  SCENARIOS.all.templates = [
    ...SCENARIOS.cloud.templates,
    ...SCENARIOS.kernel.templates,
    ...SCENARIOS.scifi.templates,
    ...SCENARIOS.gamedev.templates,
    ...SCENARIOS.developer.templates
  ];

  function formatTimestamp(d = new Date()) {
    const pad = (n, s = 2) => String(n).padStart(s, '0');
    const Y = d.getFullYear();
    const M = pad(d.getMonth() + 1);
    const D = pad(d.getDate());
    const h = pad(d.getHours());
    const m = pad(d.getMinutes());
    const s = pad(d.getSeconds());
    const ms = pad(d.getMilliseconds(), 3);
    const us = pad(Math.floor(Math.random() * 1000), 3);
    return `${Y}-${M}-${D} ${h}:${m}:${s}.${ms}${us}`;
  }

  function generateLogLine(scenarioKey = 'all') {
    const scenario = SCENARIOS[scenarioKey] || SCENARIOS.all;
    const templateFn = randomItem(scenario.templates);
    const content = templateFn();
    const ts = formatTimestamp();
    return `${ts} ${content}`;
  }

  // 緊急復旧失敗シークエンス用のログジェネレーター
  function generateEmergencyFixSequence() {
    const steps = [
      `[INFO] [HOTFIX] Emergency patch initiation requested by operator...`,
      `[INFO] [HOTFIX] Analyzing system faults: 41,209 critical anomalies found`,
      `[INFO] [HOTFIX] Applying automated quickfix patch "fix-production-pls-work-v2.patch"...`,
      `[INFO] [HOTFIX] Progress: 12%... Memory stabilizing...`,
      `[INFO] [HOTFIX] Progress: 48%... Database locks releasing...`,
      `[INFO] [HOTFIX] Progress: 89%... CPU temperature normalizing...`,
      `[INFO] [HOTFIX] Progress: 99%... Reconnecting services...`,
      `[WARN] [HOTFIX] Warning: Undocumented dependency detected: 'left-pad-turbo-unstable'`,
      `[CRITICAL] [HOTFIX] ERROR IN PATCH: Variable 'fix' was undefined at runtime!`,
      `[FATAL] [HOTFIX-DISASTER] HOTFIX HAS CAUSED A RECURSIVE CASCADE MELTDOWN!`,
      `[EMERGENCY] ALL SYSTEMS ARE DOWN! ERROR PRODUCTION MULTIPLIED BY 10X!`,
      `[PANIC] YOU MADE IT WORSE! WHY DID YOU PRESS THE EMERGENCY FIX BUTTON?!`
    ];
    return steps.map(s => `${formatTimestamp()} ${s}`);
  }

  // 自爆シーケンス
  function generateSelfDestructSequence() {
    return [
      `[EMERGENCY] ⚠️ SELF-DESTRUCT COMMAND ACCEPTED BY ROOT CONSOLE ⚠️`,
      `[ALERT] Authorization code: 0-0-0-DESTRUCT-0 validated.`,
      `[ALERT] Reactor overload countdown initiated: T-minus 5...`,
      `[ALERT] T-minus 4... Flushing coolant into secondary vacuum chambers...`,
      `[ALERT] T-minus 3... Warning: Backup servers also detonating...`,
      `[ALERT] T-minus 2... Farewell, cruel world of microservices...`,
      `[ALERT] T-minus 1... Core temperature exceeded 10,000,000 Kelvin!`,
      `[FATAL] 💥 BOOM! COMPLETE SYSTEM DESTRUCTION OCCURRED 💥`,
      `[SYSTEM] Total files destroyed: 1,940,291. Have a nice day.`
    ].map(s => `${formatTimestamp()} ${s}`);
  }

  return {
    SCENARIOS,
    generateLogLine,
    generateEmergencyFixSequence,
    generateSelfDestructSequence,
    formatTimestamp
  };
});
