#!/bin/bash

generate_farm_vars() {
  local namespace=$1
  if [[ -z "$namespace" ]]; then
    echo "Error: Namespace is required." >&2
    return 1
  fi

  local ip
  ip=$(minikube ip)
  if [[ -z "$ip" ]]; then
    echo "Error: Failed to get Minikube IP." >&2
    return 1
  fi

  local ad_port
  ad_port=$(kubectl --namespace "$namespace" get services adproxy-loadbalancer \
    --output jsonpath="{.spec.ports[0].nodePort}" 2>/dev/null)

  local dm_port
  dm_port=$(kubectl --namespace "$namespace" get services daemon-loadbalancer \
    --output jsonpath="{.spec.ports[0].nodePort}" 2>/dev/null)

  if [[ -z "$ad_port" ]]; then
    echo "Error: 'adproxy-loadbalancer' service not found in namespace '$namespace'." >&2
    return 1
  fi
  if [[ -z "$dm_port" ]]; then
    echo "Error: 'daemon-loadbalancer' service not found in namespace '$namespace'." >&2
    return 1
  fi

  local suffix="${namespace//-/_}"
  echo "ad_${suffix}=http://$ip:$ad_port"
  echo "dm_${suffix}=http://$ip:$dm_port"
}

generate_farm_vars "adition-farm1"
generate_farm_vars "adition-farm2"
