#! /bin/sh

\. env/bin/activate
mkdir -p clase01a/animations

for selection in elitist random best-with-all best-with-worse tournament roulette; do

  python -m clase01a animate --elite-size 0 --frame-duration 100 --generations 200 --generation-divider 5 --output "clase01a/animations/$selection.webm" --selection-type "$selection" --x-limits -120 120
done

for selection in elitist random best-with-all best-with-worse tournament roulette; do

  python -m clase01a animate --elite-size 2 --frame-duration 100 --generations 200 --generation-divider 5 --output "clase01a/animations/$selection-elite.webm" --selection-type "$selection" --x-limits -120 120
done