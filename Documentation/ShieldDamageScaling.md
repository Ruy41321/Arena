# Shield Damage Scaling

How incoming damage is split between the **Shield** and **Health** bars.

This document is the single source of truth for the mitigation model: `GASArchitecture.md` and
`DamageAndStatusEffects.md` describe *where* the split happens in the damage pipeline and point here
for *how* it is computed.

---

## 1. Design intent

The Shield is **armor, not a second health bar**. It scales down the damage that reaches Health, and
pays for that protection out of its own pool. Three properties are required of the model:

1. **The Shield must always break before Health runs out.** A shield that outlives the health bar
   makes the player die "with armor still on" and feels worthless.
2. **The wear must accelerate as the shield degrades.** A battered shield protects less, so more
   damage gets through and the remaining pool burns faster — the pool visibly collapses at the end.
3. **The two bars must rebalance themselves.** Restoring a full shield while at low health must
   translate into the shield sacrificing itself, not into the player dying with a nearly full shield.

An earlier model made the shield pay exactly what it absorbed. Since absorption grew with the
remaining shield, the pool decayed *asymptotically*: mathematically it could never empty, and Health
always ran out first. The current model fixes this by construction rather than by tuning.

---

## 2. The algorithm

Everything derives from one rule:

> **In percentage of their own maximum, the Shield drains `DrainRatio` times faster than Health.**

Given a raw damage `D`, current values `S`/`H` and maximums `MS`/`MH`:

```
s = S / MS                                                  // shield fraction
h = H / MH                                                  // health fraction

R = DrainRatio * clamp(s / h, 1, MaxBoost)                  // 1. drain ratio, boosted when unbalanced
k = 1 + DegradationPenalty * (1 - s)                        // 2. wear factor, rises as the pool empties

Absorbed     = D * (R * MS) / (k * MH + R * MS)             // 3. solve the split for the ratio R
HealthDamage = D - Absorbed
ShieldDamage = min(k * Absorbed, S)
```

### Step 1 — Drain ratio `R`

`R` is the pacing knob and reads directly as design intent. The boost `s / h` only kicks in when the
shield is *proportionally* healthier than health (`s > h`), which is exactly the "shield potion at low
health" case: the shield then spends itself several times faster to pull the two bars back in line.
During a normal fight the shield is always the bar that is further down, so the boost stays at `1` and
the drain ratio is simply `DrainRatio`.

### Step 2 — Wear factor `k`

`k` is the cost in pool points of one point of absorbed damage. At full shield `k = 1`; as the pool
empties it rises to `1 + DegradationPenalty`, so a damaged shield converts its remaining points into
protection less efficiently. This is what makes the mitigation decay and the chip grow over a fight.

**`k` does not affect the depletion race** — it cancels out of the normalized ratio (see §3), so it can
be tuned purely for feel without touching the guarantee.

### Step 3 — The split

`Absorbed` is the solution of the equation that expresses the rule above:

```
(ShieldDamage / MS) / (HealthDamage / MH) = R
```

Health takes what is left, and the shield is charged `k * Absorbed`, clamped to the remaining pool.

### Shield Break

If `D >= S * ShieldBreak_DamageMultiplier` the hit is still mitigated normally, but the whole pool
shatters instead of being chipped: heavy blows strip the armor outright.

---

## 3. Invariants

These are properties of the formula, not of the tuning — they hold for any hit size, any number of
hits and any pool sizes, up to the caveats listed below the table:

| # | Invariant | Consequence |
|---|---|---|
| 1 | `(ShieldDamage / MS) / (HealthDamage / MH) == R` | The damage reduction and the wear factor cancel out of this quotient. |
| 2 | A full shield breaks when Health reaches `1 - 1/DrainRatio` of its maximum | With `DrainRatio = 2` the shield is gone at 50 % health. When the bars deplete together from full, Health is never the one that empties first. |
| 3 | `HealthDamage > 0` whenever `D > 0` | The boost is capped by `MaxBoost`, so mitigation tops out at ~95 %: no amount of shield grants immortality. |
| 4 | The split is scale-invariant in `D` | Ten small hits and one big hit consume the same fraction of both pools. |

Invariant 2 is the one that was missing before and is the reason the model is expressed as a *ratio of
normalized drain rates* rather than as an absorption curve.

### Caveats

- **Invariant 1 bends on the breaking hit.** `ShieldDamage` is clamped to the remaining pool, so on
  the hit that empties the shield the normalized quotient drops below `R`. Every earlier hit satisfies
  it exactly.
- **Invariant 2 is not an absolute "never".** The rebalancing boost is capped at `MaxBoost`, so if
  Health is already more than `MaxBoost` times lower (in fraction) than the shield — e.g. a shield
  potion drunk at near-death — Health can still empty first. This is the same cap that enforces
  invariant 3, and the shield-potion table in §5 shows it: Health lands at 1 just as the shield runs
  out.
- **Invariant 3 holds for the computed split, not after rounding.** The deltas applied to the
  attributes are rounded to whole points, so a computed `HealthDamage < 0.5` lands as 0: a stream of
  small hits at maximum boost can leave Health untouched while the shield lasts.

---

## 4. Where it lives in the code

All of it is contained in `UMKHAttributeSet` — no Blueprint, data asset or gameplay effect is involved.

| What | Where |
|---|---|
| Tuning constants | [MKHAttributeSet.cpp:189](../Source/Makhia/Private/AbilitySystem/Attributes/MKHAttributeSet.cpp#L189) |
| Drain ratio (step 1) | `CalculateShieldDrainRatio` — [MKHAttributeSet.cpp:200](../Source/Makhia/Private/AbilitySystem/Attributes/MKHAttributeSet.cpp#L200) |
| Wear factor (step 2) | `CalculateShieldWearFactor` — [MKHAttributeSet.cpp:207](../Source/Makhia/Private/AbilitySystem/Attributes/MKHAttributeSet.cpp#L207) |
| The split (step 3) | `CalculateDamageSplit` — [MKHAttributeSet.cpp:212](../Source/Makhia/Private/AbilitySystem/Attributes/MKHAttributeSet.cpp#L212) |
| HUD read-out of the current reduction | `GetShieldDamageReduction` (BlueprintPure) — [MKHAttributeSet.cpp:239](../Source/Makhia/Private/AbilitySystem/Attributes/MKHAttributeSet.cpp#L239) |
| Entry point / shield break branch | `HandleIncomingDamage` — [MKHAttributeSet.cpp:248](../Source/Makhia/Private/AbilitySystem/Attributes/MKHAttributeSet.cpp#L248) |
| Application to the attributes | `ApplyShieldBreak` / `ApplyShieldDamageMitigation` — [MKHAttributeSet.cpp:410](../Source/Makhia/Private/AbilitySystem/Attributes/MKHAttributeSet.cpp#L410) |

Runs **server-side only**, inside `PostGameplayEffectExecute`, after `UExecCalc_Damage` has produced
the final `IncomingDamage` (buffs, crit and mitigation modifiers are already applied at that point).

### Tuning constants

| Constant | Default | Effect |
|---|---|---|
| `ShieldWear_DrainRatio` | `2.0` | *The* pacing knob: the shield breaks when Health is at `1 - 1/Ratio` of its max. `3.0` → breaks at 66 % health, mitigating less overall. |
| `ShieldWear_DegradationPenalty` | `1.0` | How much the mitigation decays over the life of the pool. `0` → constant reduction; `1` → the reduction roughly drops by a third before breaking. |
| `ShieldBalance_MaxBoost` | `10.0` | Ceiling of the rebalancing response, and therefore of the mitigation (~95 %). |
| `ShieldBreak_DamageMultiplier` | `2.0` | Hits above `Shield × this` shatter the pool outright. |

---

## 5. Balance tables

Typical in-game ranges: pools between 100 and 150 even with buffs, hits between 10 and 30.
All values below use the defaults of §4.

### Single hit — damage 20, `MaxHealth = MaxShield = 100`

| Shield | Health | → Health | → Shield | Reduction |
|---|---|---|---|---|
| 100 | 100 | 6.7 | 13.3 | 67 % |
| 80 | 90 | 7.5 | 15.0 | 63 % |
| 60 | 80 | 8.2 | 16.5 | 59 % |
| 40 | 65 | 8.9 | 17.8 | 56 % |
| 20 | 55 | 9.5 | 18.9 | 53 % |
| 100 | 50 | 4.0 | 16.0 | 80 % |
| **100** | **20** | **1.8** | **18.2** | **91 %** |
| 40 | 20 | 5.7 | 22.9 | 71 % |

The last three rows are the rebalancing at work: the lower Health is relative to the Shield, the more
the shield sacrifices itself. Note how at Shield 40 / Health 20 the shield pays 22.9 for 14.3 points of
absorbed damage — it is spending its pool inefficiently *and* fast, because it is the only bar that can
afford it.

### Same state (100 / 100), varying damage — the split is a constant share

| Damage | → Health | → Shield | Reduction |
|---|---|---|---|
| 10 | 3.3 | 6.7 | 67 % |
| 15 | 5.0 | 10.0 | 67 % |
| 20 | 6.7 | 13.3 | 67 % |
| 25 | 8.3 | 16.7 | 67 % |
| 30 | 10.0 | 20.0 | 67 % |

### Full fight — `Max 100/100`, hits of 20

| Hit | → Health | → Shield | Health | Shield | |
|---|---|---|---|---|---|
| 1 | −7 | −13 | 93 | 87 | |
| 2 | −7 | −14 | 86 | 73 | |
| 3 | −8 | −16 | 78 | 57 | |
| 4 | −8 | −17 | 70 | 40 | |
| 5 | −9 | −18 | 61 | 22 | |
| 6 | −9 | −19 | 52 | 3 | |
| 7 | −10 | −3 | 42 | 0 | **shield break** |
| 8 | −20 | 0 | 22 | 0 | |
| 9 | −20 | 0 | 2 | 0 | |
| 10 | −20 | 0 | 0 | 0 | |

The shield empties on hit 7 with Health at ~50 % (invariant 2), and the per-hit chip grows from 13 to
19 as it degrades. Without any shield the same fight lasts 5 hits, so a full pool is worth **+100 %
survivability**.

### Shield potion at low health — `Max 100/100`, hits of 20

Health at 20, shield restored to 100:

| Hit | → Health | → Shield | Health | Shield | |
|---|---|---|---|---|---|
| 1 | −2 | −18 | 18 | 82 | |
| 2 | −2 | −21 | 16 | 61 | |
| 3 | −3 | −24 | 13 | 37 | |
| 4 | −4 | −25 | 9 | 12 | |
| 5 | −8 | −12 | 1 | 0 | **exhausted** |
| 6 | −20 | 0 | 0 | 0 | |

The shield takes ~90 % of everything and burns itself out in 5 hits, while Health drops by 2 per hit
instead of 7. Without the rebalancing boost the same situation would have killed the player with the
shield still at ~80.

### Buffed pools — `Max 150/150`, hits of 30

| Hit | → Health | → Shield | Health | Shield | |
|---|---|---|---|---|---|
| 1 | −10 | −20 | 140 | 130 | |
| 3 | −12 | −23 | 117 | 85 | |
| 5 | −13 | −27 | 91 | 33 | |
| 7 | −15 | −5 | 62 | 0 | **shield break** |
| 10 | −30 | 0 | 0 | 0 | |

Same shape as the 100/100 fight: the model is scale-invariant, so buffing both pools does not change
the pacing, only the number of hits.

### Asymmetric pools — `MaxHealth 100`, `MaxShield 60`, hits of 20

| Hit | → Health | → Shield | Health | Shield | |
|---|---|---|---|---|---|
| 1 | −9 | −11 | 91 | 49 | |
| 3 | −11 | −13 | 70 | 24 | |
| 5 | −12 | −10 | 47 | 0 | **shield break** |
| 8 | −20 | 0 | 0 | 0 | |

A smaller shield mitigates less per hit and breaks sooner in absolute terms, but still at ~50 % health:
invariant 2 does not depend on the pool sizes.
