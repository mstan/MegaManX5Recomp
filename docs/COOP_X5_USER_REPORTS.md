# X5 user-reported issues and current QA build

Owner scope, 2026-10-25: reconcile the reports from this session and deliver
the current local build. The original comparative parity campaign is parked;
its remaining comparisons are not release requirements for this handoff.

Local release: **v0.2.1-coop-alpha-debug / 4EBA758E**, retaining the accepted
11A66B52 gameplay with a release version. The owner explicitly accepted the
duel's flash-time disappearance and confirmed physical keyboard/DualSense
isolation. The recent follow-up burndown is **8 resolved / 2 deferred / 10**.
The two deferred findings need a normal-route intro recording and a precise
solo Grizzly location/recording; neither is claimed fixed. LAN remains deferred.
Release evidence: `G:/Temp/Codex/psxrecomp/x5-parity/owner-release-20261026`.

Prior candidate: **11A66B52**. The owner1864D0A4 retest accepted the
sequence as close, but requested earlier disappearance into the explosions.
The loser now hides at the native white-flash handshake, above16/31 brightness,
while the explosion controller is still active and before full white. This
refines the existing defeat check below; no extra configuration matrix.
Evidence: owner-duel-flash-20261026. The owner subsequently accepted this
defeat sequence. That acceptance does not qualify complete stories or endings.

Prior candidate1864D0A4 replaced ordinary death orbs with the
owner's duel-only crouch, explosion, then vanish sequence. A real accepted
fatal selects the native player injury pose51, holds the loser visibly through
the original class1A blast, then hides the body after that controller retires.
The defeat hold uses the native arena floor. The winner retains HP
and result ownership; no ordinary game-over or team retry is requested.
Current-build crouch save/load uses the existing serialized body/lifecycle
state. No new save fields or native-card format changes are needed.
Both grounded native NPC/entry paths pass. A single airborne setup did not
produce an accepted hit, so airborne final-hit/settling remains unverified;
that failed setup is retained rather than counted as a pass.

Prior candidate8EA4F4CA corrected duel defeat ownership after
the owner's9D02E26B report. The losing participant's meter empties while the
winner's retains actual HP. The native explosion controller is placed at the
fallen loser's coordinates; its timers and story handshake remain native.
The entrant's own HUD body/art stays selected even when P1 dies and P2 owns
the surviving world. No ordinary game-over or shared retry is requested.
Five actual final-hit checks and all24 CTests pass: either winner on both
native NPC/entry paths, plus a17HP boss winner. Native result dialogue,
unchanged retries and zero local-view leaks are retained. Owner visual
acceptance and later story/native-save/LAN gaps remain open.

This retains9D02E26B's corrections to duel arrival and HUD rendering
after the owner's82A36EAA failures. The real Zero arrives during the native
conversation; both32HP meters appear together at control release, with matching
anchors and no inherited96HP red trail. P2 entrants use their own HUD tiles
inside the existing private render transaction. Actual entry, movement,
P2 menu/save/load and a real2HP X hit pass on this candidate. Owner visual
acceptance remains pending; complete winner/story/save coverage is not claimed.
Both entering-seat/native-NPC paths and all24 title CTests passed on9D02E26B.

This retains0ACB3571's explicit-unbound keyboard fix (SDK59cae0a9) and intro
moving-block last-living-player rule. Its24 title CTests and SDL2/SDL3 binding
regressions passed. Native moving-block checks passed all five cases. An actual Windows/SDL
keyboard test moves only X53.5px while Zero stays still, with all debug
input overrides cleared. The owner subsequently confirmed physical DualSense
isolation on11A66B52. Evidence is in owner-input-walls-20261026 and the release
owner-acceptance.json.
Prior delivered packages82A36EAA and0ACB3571 are historical candidates.

Owner82A36EAA station notes were read and frozen at
`G:/Temp/Codex/psxrecomp/x5-parity/owner-input-walls-20261026/owner-observations-82A36EAA.json`.
Grizzly graphics/deaths, Gaea graphics and solo Alia received Pass. The
large-health aid apparently only lowered Zero's HP: that does not validate a
human pickup test. Keyboard and DualSense each drove both heroes, invalidating
inferences about independently controlled ownership from this run. The duel
received Fail with invisible arrival and incorrect meters. LAN is deferred.

**Not every report is closed.** The new solo background pause and separate
original Both-door X fatal remain deferred. Duel defeat and physical input
isolation are accepted. Duel progression/save limits remain below.

## Report inventory

"Bounded pass" means the actual reported mechanic was exercised within the
retained evidence, not every stage or network condition. Older technical
passes are identified as such; they are not new82A36EAA owner confirmations.

| Report | Implemented fix / evidence | Remaining acceptance |
|---|---|---|
| Start shows Zero in both panes instead of each hero's own menu. | Fixed; native independent menus, cursors, equipment, resume and snapshot checks passed. Local issue .10.8, source460450d; current local peer regression also passes independent menus. | Original physical visual confirmation is not explicitly recorded. |
| Large Zero health pickup briefly shows X's camera. | Fixed; late native large-heal captures preserve distinct views, source460450d/.10.9. Current nonblocking pickup check also retains each camera and resource owner. | Original physical symptom retest is not explicitly recorded. |
| Distant Zero intro progress corrupts X's scenery. | Fixed; retained per-view native atlas, original trigger and survivor art exercised, source460450d/.10.10. | Owner later reported no other intro issues, but did not explicitly repeat this exact trigger. |
| X's final death corrupts the screen after Zero died far ahead. | Fixed; original trigger, native fatal, team wipe, fade and retry qualified, source98c8e16/.10.12. | Targeted owner confirmation remains unrecorded. |
| Zero's jump then air-dash trail is garbled. | Fixed by f651130; actual native dash exhaust and afterimages render cleanly in both panes on82A36EAA. Existing .10.11, Q03/R9. | Owner82A36EAA Grizzly Pass: graphics OK after the explicit effect instructions. |
| Duplicate intro counterpart NPC; redundant reunion dialogue. | Fixed; co-op skips the authored reunion while retaining Sigma's introduction. Owner explicitly confirmed the skip, .10.13. | No open report-specific implementation gap. |
| Grizzly black bars and scrolling reset. | Fixed; native tile-ring wrapping exercised in actual private views, source98c8e16/.10.14. | Owner later said Grizzly works; that is broad feedback, not an explicit repeat of every original wrap interval. |
| Ahead Zero dies near moving intro blocks when X approaches. | Fixed; original native supports and linked pieces now admit P2; Zero crosses the original fatal region without immunity, source1a8d11c/.10.15. | Targeted owner route confirmation remains unrecorded. |
| Trailing X dies when Zero enters the opening Sigma room. | Fixed; scene-owner camera projection preserves X through entry. Owner subsequent native route retains X HP and reports no other intro issues, source640bfed/.10.16. | The later Both-door finding below is separate. |
| Intact statue missing before explosions/Sigma reveal. | Fixed; original camera mode is published to retained views, source8b23703/.10.17. Original owner route/replay supplies intact statue, explosion and reveal evidence. | Current candidate has not received a separate owner reveal-only confirmation. |
| Sigma invisible in P2's view. | Fixed; original Sigma layer fields projected inside the private draw transaction. Exact owner replay and composed before/after images qualified, source17e3475/.10.19. | Owner visual retest remains unrecorded. |
| Zero draws over X's face. | Fixed; keep X's native base/armor sequence together. Qualified alongside the Sigma visibility fix, source17e3475/.10.19. | Owner visual retest remains unrecorded. |
| Duel opponent meter starts oversized and is vertically misaligned. |9D02E26B withholds both meters during the native fill, bounds the inherited red trail to32 and retains aligned native packets. Actual P2 entrant captures show the correct X badge and both green meters. .10.18. | Owner82A36EAA Fail supersedes the older automated claim. Current correction passes bounded automation; owner visual acceptance pending. |
| Duel Zero aura remains after moving. | Retire only the native NPC shell's owned aura. Current native intro/control captures retain the real participant without a stationary shell/aura. .10.18. | Current appearance/balance acceptance pending; no full duel completion claim. |
| Duel NPC appears, vanishes, then player teleports in again. |9D02E26B uses the native hovering Zero's standing box and the actual entrant's floor to start one real arrival during dialogue; the native shell's intro/physics continue privately. .10.18. | Current page0 already shows both heroes; controls release at32HP each. Owner visual acceptance pending. |
| Duel needs fair PvP health/damage while retaining normal moves and menus. | Implemented equal32HP, scoped damage tuning, no healing/bonuses; real contact, menus and active snapshots qualified, .10.18. | Human fight-feel/balance and physical LAN acceptance remain open. |
| When the entrant loses, the living boss player's meter empties too. |8EA4F4CA projects the real duel health ledger during the native HUD call, preserving the shell's private defeat state and selecting the entrant's own body through survivor handoff. .10.18. | Bounded real-fatal automation passes; owner acceptance pending. |
| Duel explosions remain at the boss even when that player wins. |8EA4F4CA relocates only the newly authored class1A defeat controller to the actual fallen loser before its first update. Native effects/timers/story signals continue. .10.18. | Bounded real-fatal automation passes; owner acceptance pending. |
| Ordinary death orbs play before the duel's authored explosion; loser then stays visible too long. |11A66B52 retains1864D0A4's native crouch/no-orb admission, then hides at the native white-flash handshake while explosions still run, before full white. .10.18. | Four real final-hit checks pass; owner accepted the resulting sequence. Retained in4EBA758E. |
| Grizzly electric chains are not animated. | Fixed; original animated source rectangles advance in both panes, G05/.10.20. | Owner co-op Grizzly run later reported working. |
| Both-live players bypass train-front camera stops/cannons. | Fixed; native camera command ownership shared correctly. Owner accepted actual co-op cannon/stop/release run, G02/.10.20. | No open co-op report-specific gap; solo hint behavior is separate. |
| Hidden Alia messages leave the cannon impossible to destroy, including solo. | Fixed native hint acknowledgement/side effects; co-op cannon destruction accepted by owner, G01/.10.20. | Owner82A36EAA solo Pass: ?alia is fixed?; focused G01 accepted. A new background-scroll pause is separate. |
| P2 enemy-hit effects are corrupted. | Fixed; native damage and retained static impact art qualified, G06/.10.20. | Other new projectile reports are tracked separately below. |
| P2 does not start the exploding-train timer. | Fixed; actual native admission, countdown and explosion effects qualified, G03/.10.20. | Timed normal-route two-person/LAN play is separate acceptance. |
| P2's final escape train never appears. | Fixed; native train spawn, boarding, continuation/checkpoint and partner return qualified, G04/.10.20. | Normal timed-route/LAN play is separate acceptance. |
| Either leading player can leave the other invisible at Grizzly's boss door. | Fixed; actual intro and visible passenger/control return passed for each true leading seat, G08/.10.20. | No open report-specific implementation gap. |
| P2 briefly shows P1's HUD after the capsule cutscene. | Fixed; actual native part grant and own HUDs during unlocked return qualified, G07/G09/.10.20. | No open report-specific implementation gap. |
| X/P2 needs armor selection; both heroes should retain Fourth/Z-Buster availability. | Implemented X-owned stage armor selection, P1 frontend ownership and co-op bonuses, with actual native new games/entries in both seat assignments, .10.21. | **Actual X/P2 armor selection over physical LAN remains unverified.** |
| Health pickups briefly pause both heroes/world. | Fixed by c32de77; large P2/small P1 healing continues while the partner jumps. Exact amounts, resources, cameras and collector recovery pass, Q01/.10.22 closed. | No open bounded mechanic check; physical LAN remains separate. |
| Zero's Triangle projectile is garbled. | Fixed by f651130; actual blue Z-Buster renders cleanly in both panes on82A36EAA, Q02/.10.23. | Owner82A36EAA Grizzly Pass: graphics OK after the explicit effect instructions. |
| Gaea X's attack/muzzle art is garbled in both panes. | Fixed by f651130; retain static shot origin and original compressed atlas uploads. Actual gold muzzle/green projectile clean on82A36EAA, A05/.10.21. | Owner82A36EAA Gaea Pass. |

The earlier generic closing-walls report was withdrawn. The owner has now
specified a new intro roof-climb moving-block policy (.10.28): all living
heroes must pass; death or withdrawal releases the survivor. This is separate
from boss-door admission. Capsules retain their existing grant/control evidence.

| New report | Current action | Status |
|---|---|---|
| Keyboard and DualSense each control both heroes (.10.27). | Remove QA PSX_DEV_INPUT any-controller merge; SDK59cae0a9 honors fully unbound keyboard slots. | SDL2/SDL3 binding regression and actual OS keyboard isolation pass. Owner confirms physical devices stayed separate; closed. |
| Intro roof blocks close before the trailing hero passes (.10.28). | Gate original F65C8 admission on every living hero; retain native motion and linked pieces. Dead/withdrawn teammates do not block. | Passed five native checks: either leader waits, both cross, either survivor after death, P2 withdrawal. All three structures and both linked pieces actually move; sub1 finishes atX4232 and linked pieces retire. |
| Solo Grizzly background intermittently pauses scrolling (.10.29). | Native layer1/2 scroll advances in every sampled advancing-frame interval during a900-frame stationary/Right check. Exact old replay diverged at413 and is not accepted as route evidence. | Unreproduced in the bounded check; deferred for an owner recording/location. No fix or complete-stage pass claimed. |

Owner passed the intro Either/Both station entries. Because controls were
coupled, these are broad visual feedback and do not close the independently
controlled Both-door fatal or prove the new block admission requirement.

## Open finding and duel feature limits

Local issue .10.25: the owner's33E9009F opening-boss recording also contains a first-attempt X
fatal under Both-player doors. It remains present in the original and the
render-corrected replay; the Sigma rendering fix did not repair it. This is
an **open, unreproduced finding**, not a claim that it reproduces on the current candidate.
The new roof policy keeps the lower partner alive while the upper hero waits.
Attempts to continue a direct-CP2 Both setup could not produce a qualified
native boss admission: one waited at the aperture and another rejected the
follow-up teleport. These are setup failures, not boss passes. A speculative
Sigma admission guard was removed without shipping it. Resume from a normal
owner route/recording rather than adding more setup variants.
Default Either-player doors are retained as requested. The separate earlier
room-entry fatal above must not be used to close this finding.
The release4EBA758E follow-up bypassed the earlier route from CP0 rather than
loading CP2 directly. Both heroes survived the sampled separation, but the
original roof structures/boss admission were not present in that setup.
This is another setup failure, not a Both-door pass. No speculative code is
shipped; further automated setup variants are stopped pending a normal route.

Release4EBA758E solo Grizzly evidence retains the uninterrupted live prefix
with continuous background scrolling and32HP. The longer requested run reaches
ordinary pit deaths and retry/loading intervals, then times out. The raw
failed1200-frame observer is retained separately from the qualified prefix;
no900/1200-frame pass is claimed on this candidate. The earlier900-frame
evidence is historical. No scrolling pause was reproduced or fix claimed.
Owner has not retested solo.

The duel winner requirement is only partially validated: one actual fatal
selected the expected native result, but both winner stories/endings and
physical LAN are not fully qualified. Native cold-save winner persistence
is **not implemented**. These are feature-completion gaps, not new symptoms
invented for a regression matrix. Current-build active save/load remains
separate and qualified.

## Current bounded follow-up burndown

These ten checks describe the recent follow-ups, not the parked comparison campaign.
Eight corrections are resolved; two checks remain deferred for reproduction.

| Check | Status and evidence |
|---|---|
| Zero invisible until duel control release | Passed both actual inner-door paths. The hovering-Zero path has both heroes visible on page0; one real arrival, native dialogue continues. combined-p1/p2. |
| Oversized/red-draining duel opponent meter | Passed: both start32HP; inherited native48/96HP trail bounded to32. First fight captures show aligned full meters. |
| Green entrant meter lingers during duel dialogue | Passed composed dialogue captures: both meters withheld until native controls release. |
| P2 entrant HUD loses bar/badge or shows the other hero's art | Passed composed X/P2 capture and active movement/menu/save/load/actual2HP hit; zero local-view state leaks. combined-active-p2. |
| Living opponent's meter empties when entrant loses | Corrected8EA4F4CA. Actual final hits empty only the loser and retain winner HP; no shared retry/game-over. owner-duel-defeat-20261026. |
| Boss-side explosion effects play even when that player wins | Corrected8EA4F4CA. Original defeat emitter uses the fallen loser's actual position; native result text/handshake still runs. owner-duel-defeat-20261026. |
| Ordinary death orbs precede duel explosion; disappearance timing | Refined11A66B52 after owner1864D0A4 feedback: native crouch, no ordinary orbs, vanish at the original flash handshake while explosions still run. owner-duel-flash-20261026. |
| Keyboard/DualSense isolation acceptance | Passed OS keyboard and SDL2/SDL3 regressions; owner explicitly confirms physical devices stayed separate on11A66B52. Closed. |
| Original Both-door intro X fatal | Deferred: normal-route reproduction required; direct-checkpoint continuation did not qualify boss admission. No speculative timing fix shipped. .10.25. |
| Solo Grizzly background pause | Deferred/unreproduced:900 native frames with continuous sampled layer scroll. A recurrence needs location/action and recording. .10.29. |

Owner accepted the current duel defeat sequence; competitive balance is not
certified. LAN is deferred. Native cold-save winner persistence and complete
winner/story/ending qualification remain the separate feature gaps above.

## Short retest bundles

1. Test Grizzly Stage.cmd: Zero/P2 Triangle, jump then air dash, and a large
   health pickup while the keyboard hero moves. Optional continuation can
   check the earlier camera, scenery, HUD and train fixes in one run.
2. Test Gaea Armor.cmd: normal X/P2 shots and muzzle flashes in both panes.
3. Solo with co-op disabled and Alia hidden: destroy the actual main cannon.
4. One matched-build LAN session: Zero/P1 campaign, X/P2 owns armor selection,
   P1 owns frontend menus, both retain their seats on stage entry.
5. Test X vs Zero Duel.cmd: meter, one arrival, no stationary aura, movement,
   attacks, both menus and fight feel. Story/save/ending gaps remain explicit.

Retained evidence is indexed by COOP_X5_POST_GRIZZLY_QA.md,
COOP_X5_GRIZZLY_BURNDOWN.md, COOP_X5_ARMOR_SELECT.md and the earlier user-report
sections of COOP_PARITY_BURNDOWN.md. The latter is historical evidence only;
its old campaign inventory is outside this handoff's scope.

Current follow-up evidence:
`G:/Temp/Codex/psxrecomp/x5-parity/owner-followups-20261026`.
The combined-p2 entry and combined-active-p2 check use9D02E26B. The latter
checks actual native movement, an interactive P2 menu, current-build active
save/load, a real2HP X shot and zero local-view state leaks. Solo and Both-door
limits are retained with their reports; neither is silently closed.

The task-created49,320,168-byte G: sector cache was deleted using native
Remove-Item -LiteralPath without Force and verified absent. No native saves,
owner settings, recordings or requested deliverables were deleted.
