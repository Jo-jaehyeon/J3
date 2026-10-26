#!/usr/bin/env bash
# Copyright (c) ACCENT. All Rights Reserved.
#
# 단계 2 공장 시뮬레이션·복제 벤치마크 (Git Bash).
# 리슨 서버 하나를 화면 없이(-nullrhi) 띄워 벤치마크 컨베이어를 깔고, 잠시 뒤 클라이언트 하나를 따로 실행해 중도 참가시킨다.
# 두 프로세스의 [Bench] 로그를 모아 출력한다.
#
# 사용: Tools/Bench/factory_bench.sh <구간 수> <구간당 아이템 수> <Iris 1|0> <보정 주기 초> [측정 시간 초] [클라 참가 지연 초]
# 예:   Tools/Bench/factory_bench.sh 2000 10 1 0.5 40 15
#
# 주의: 에디터를 닫고 실행한다. 보정 주기는 Data/config.csv의 factory_resync_sec를 잠시 바꿨다가 되돌린다.

set -u

SEGMENTS=${1:?구간 수}
ITEMS=${2:?구간당 아이템 수}
IRIS=${3:?Iris 1 또는 0}
RESYNC=${4:?보정 주기 초}
DURATION=${5:-40}
CLIENT_DELAY=${6:-15}

PROJECT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
UPROJECT="$(cygpath -w "$PROJECT_DIR/J3.uproject")"
EDITOR="${UE_EDITOR:-/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe}"
OUT_DIR="$PROJECT_DIR/Saved/Bench"
TAG="s${SEGMENTS}_i${ITEMS}_iris${IRIS}_r${RESYNC}"
CONFIG="$PROJECT_DIR/Data/config.csv"

mkdir -p "$OUT_DIR"
SERVER_LOG="$OUT_DIR/${TAG}_server.log"
CLIENT_LOG="$OUT_DIR/${TAG}_client.log"
rm -f "$SERVER_LOG" "$CLIENT_LOG"
SERVER_LOG_WIN="$(cygpath -w "$SERVER_LOG")"
CLIENT_LOG_WIN="$(cygpath -w "$CLIENT_LOG")"


# 보정 주기 임시 변경 (끝나면 되돌림)
cp "$CONFIG" "$CONFIG.bench_backup"
trap 'mv -f "$CONFIG.bench_backup" "$CONFIG"' EXIT
sed -i "s/^factory_resync_sec,[^,]*,/factory_resync_sec,${RESYNC},/" "$CONFIG"


COMMON=(-game -nullrhi -nosound -nosplash -unattended -NoVerifyGC
	"-ini:Engine:[SystemSettings]:net.Iris.UseIrisReplication=${IRIS}"
	"-ini:Engine:[ConsoleVariables]:j3.Bench.AutoSpawn=${SEGMENTS}x${ITEMS}")


# 서버
"$EDITOR" "$UPROJECT" "/Game/J3/Maps/L_Dev_Empty?listen" "${COMMON[@]}" "-abslog=$SERVER_LOG_WIN" &
SERVER_PID=$!
sleep "$CLIENT_DELAY"


# 클라이언트 (중도 참가)
"$EDITOR" "$UPROJECT" 127.0.0.1 "${COMMON[@]}" "-abslog=$CLIENT_LOG_WIN" &
CLIENT_PID=$!
sleep "$((DURATION - CLIENT_DELAY))"


# 정리
kill "$CLIENT_PID" "$SERVER_PID" 2>/dev/null
sleep 3
taskkill //F //PID "$CLIENT_PID" >/dev/null 2>&1
taskkill //F //PID "$SERVER_PID" >/dev/null 2>&1


echo "=== $TAG ==="
grep -h -E "\[Bench\]|리슨 서버 시작|Join succeeded|참가 거부" "$SERVER_LOG" "$CLIENT_LOG" 2>/dev/null | sed -E 's/^\[[^]]*\]\[ *[0-9]+\]//'
