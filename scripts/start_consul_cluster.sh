#!/usr/bin/env bash
set -euo pipefail

# 一键启动 3 节点 Consul 集群
# 说明：
# - 节点1 暴露: 8500, 8301, 8302, 8600 到宿主机（UI/HTTP、gossip、RPC、DNS）
# - 节点2 暴露: 8501 -> 8500（仅 HTTP/UI）
# - 节点3 暴露: 8502 -> 8500（仅 HTTP/UI）
# - 2、3 节点自动 join 到 1 节点容器的 IP

CONSUL_IMAGE=${CONSUL_IMAGE:-hashicorp/consul:latest}

echo "[+] 检查 Docker..."
if ! command -v docker >/dev/null 2>&1; then
  echo "[-] 未检测到 docker，请先安装 Docker。" >&2
  exit 1
fi

echo "[+] 拉取镜像: ${CONSUL_IMAGE}"
docker pull "${CONSUL_IMAGE}" >/dev/null

# 如已存在，先移除旧容器
for name in consul1 consul2 consul3; do
  if docker ps -a --format '{{.Names}}' | grep -qx "$name"; then
    echo "[+] 移除已存在容器: $name"
    docker rm -f "$name" >/dev/null || true
  fi
done

echo "[+] 启动节点1 (consul1)"
docker run --name consul1 -d \
  -p 8500:8500 -p 8301:8301 -p 8302:8302 -p 8600:8600 \
  "${CONSUL_IMAGE}" agent -server -bootstrap-expect 2 -ui -bind=0.0.0.0 -client=0.0.0.0 >/dev/null

# 等待 consul1 就绪一小会（可按需调整/改为健康检查）
sleep 2

CONSUL1_IP=$(docker inspect --format '{{.NetworkSettings.IPAddress}}' consul1)
if [[ -z "${CONSUL1_IP}" ]]; then
  echo "[-] 获取 consul1 IP 失败" >&2
  exit 1
fi
echo "[+] consul1 容器 IP: ${CONSUL1_IP}"

echo "[+] 启动节点2 (consul2) 并加入 ${CONSUL1_IP}"
docker run --name consul2 -d \
  -p 8501:8500 \
  "${CONSUL_IMAGE}" agent -server -ui -bind=0.0.0.0 -client=0.0.0.0 -join "${CONSUL1_IP}" >/dev/null

echo "[+] 启动节点3 (consul3) 并加入 ${CONSUL1_IP}"
docker run --name consul3 -d \
  -p 8502:8500 \
  "${CONSUL_IMAGE}" agent -server -ui -bind=0.0.0.0 -client=0.0.0.0 -join "${CONSUL1_IP}" >/dev/null

echo "[+] 启动完成："
echo "    - UI: http://localhost:8500 (consul1), http://localhost:8501 (consul2), http://localhost:8502 (consul3)"
echo "    - DNS: 8600 (consul1)"
echo "[i] 查看成员: docker exec -it consul1 consul members"
echo "[i] 查看日志: docker logs -f consul1"
echo "[i] 清理集群: docker rm -f consul1 consul2 consul3"
