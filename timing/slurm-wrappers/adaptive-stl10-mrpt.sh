#!/bin/bash
#SBATCH --workdir=/wrk/hyvi/RP-test/timing
#SBATCH --job-name=adaptive-stl10-mrpt-full
#SBATCH -o script-output/adaptive-stl10-mrpt-full.txt
#SBATCH -c 1
#SBATCH -t 4-00:00:00
#SBATCH --mem=30G
#SBATCH --mail-type=END,FAIL
#SBATCH --mail-user=ville.o.hyvonen@helsinki.fi

if [ $# -ne 1 ]; then
  echo "Usage ./adaptive-stl10-mrpt.sh <post-fix>"
  exit
fi

module load GCCcore/7.3.0

srun ./grid_comparison.sh stl10 "$1"
