#!/bin/bash
# Serwer vLLM z DeepSeek-R1 (AWQ) na całym hgx2 (8x A100-80GB).
#
# Słucha na 0.0.0.0, bo eksperyment łączy się z węzła obl1 -- przy
# --host 127.0.0.1 serwer byłby osiągalny tylko z samego hgx2.
#
# Użycie (zalecane, razem z eksperymentem na obl1 i strażnikiem):
#   bash scripts/submit_llm_experiments.sh -A inz_school_tt --r1
# Sam serwer:
#   sbatch -A inz_school_tt scripts/slurm_vllm_r1.sh

#SBATCH --job-name=vllm-r1
#SBATCH --partition=hgx
#SBATCH --nodelist=hgx2
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --gres=gpu:8
#SBATCH --cpus-per-task=32
#SBATCH --mem=256G
# 3 dni eksperymentu + zapas na ładowanie modelu i kolejkę obl1 (eksperyment
# startuje najwcześniej razem z serwerem); strażnik i tak wyłączy serwer
# zaraz po końcu eksperymentu
#SBATCH --time=3-06:00:00
#SBATCH --output=slurm_vllm_r1_%j.out

PORT="${VLLM_PORT:-8000}"

source ~/miniconda3/etc/profile.d/conda.sh
conda activate vllm_env

export CUDA_HOME=$CONDA_PREFIX
export PATH=$CONDA_PREFIX/bin:$PATH
export VLLM_USE_FLASHINFER_SAMPLER=0
export HF_HOME=/raid/inz_school_tt/hf
export OMP_NUM_THREADS=8

echo "Wezel: $(hostname)  GPU: $CUDA_VISIBLE_DEVICES  Port: $PORT  Start: $(date)"
nvidia-smi --query-gpu=index,name,memory.used --format=csv
which nvcc && nvcc --version | tail -1

# --served-model-name musi się zgadzać z OPENAI_MODEL w eksperymencie
# (submit_llm_experiments.sh --r1 ustawia deepseek-r1)
vllm serve cognitivecomputations/DeepSeek-R1-AWQ \
  --served-model-name deepseek-r1 \
  --tensor-parallel-size 8 \
  --dtype float16 \
  --max-model-len 32768 \
  --gpu-memory-utilization 0.92 \
  --enable-prefix-caching \
  --trust-remote-code \
  --reasoning-parser deepseek_r1 \
  --host 0.0.0.0 \
  --port "$PORT"
