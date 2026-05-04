# 보스 죽음 처리 (GA_Boss_Death) — 흐름 가이드

이 문서는 보스가 HP 0에 도달했을 때 어떤 경로로 죽음 GA가 활성화되고, 우리 `GA_Boss_Death`가 그 위에 무엇을 얹는지 한 번에 이해하기 위한 가이드입니다.

---

## 1. 한눈에 보는 전체 흐름

```
[데미지 적용]
   │
   ▼
LyraHealthSet::PostGameplayEffectExecute
   - Health 어트리뷰트 깎임
   - Health <= 0 이면 OnOutOfHealth.Broadcast 발동
   │
   ▼
LyraHealthComponent::HandleOutOfHealth
   - ASC->HandleGameplayEvent(GameplayEvent.Death) 호출
   │
   ▼  (서버에서만)
ASC가 "GameplayEvent.Death" 트리거를 등록한 GA를 자동 활성화
   - 우리 GA_Boss_Death는 부모(ULyraGameplayAbility_Death)에서 이 트리거를 받음
   │
   ▼
ULyraGameplayAbility_Death::ActivateAbility (부모)
   ① 다른 GA들 모두 CancelAbilities (Ability_Behavior_SurvivesDeath 제외)
   ② SetCanBeCanceled(false)
   ③ ActivationGroup을 Exclusive_Blocking으로 변경 (다른 GA가 못 끼어들게 막음)
   ④ bAutoStartDeath=true → HealthComponent->StartDeath() 호출
       └ DeathState = DeathStarted
       └ OnDeathStarted.Broadcast → LyraCharacter가 콜리전/Tick 등 정리
   ⑤ Super::ActivateAbility (이후 자식 클래스의 추가 처리)
   │
   ▼
UGA_Boss_Death::ActivateAbility (자식 = 우리 코드)
   ⑥ AI Brain Stop  (보스 AIController->BrainComponent->StopLogic)
   ⑦ Movement Disable  (CharacterMovement->DisableMovement)
   ⑧ DeathMontage 재생 시작 (UAbilityTask_PlayMontageAndWait)
   │
   ▼  (몽타주 재생 종료/중단)
OnDeathMontageCompleted / OnDeathMontageCancelled
   ⑨ EndAbility 호출
   │
   ▼
ULyraGameplayAbility_Death::EndAbility (부모)
   ⑩ FinishDeath() 호출
       └ DeathState = DeathFinished
       └ OnDeathFinished.Broadcast → LyraCharacter가 Ragdoll/Destroy 처리
```

핵심 포인트는 **부모 클래스(`ULyraGameplayAbility_Death`)가 죽음 상태 머신을 자동으로 굴려준다**는 것. 우리는 그 위에 *보스 고유 처리*(AI 정지 + 몽타주)만 얹는 구조입니다.

---

## 2. 파일별 역할

### 2-1. `GA_Boss_Death.h`
- **부모**: `ULyraGameplayAbility_Death`
  - 이 클래스를 상속하기 때문에 `GameplayEvent.Death` 트리거가 자동 등록됨.
  - `ULyraGameplayAbility`만 상속하면 절대 자동 활성화되지 않음 → 죽지 않는 보스 됨. (이번에 겪었던 함정)
- 멤버 변수 `DeathMontage`: 에디터에서 죽음 애니메이션 몽타주를 지정.
- `CacheHandle / CacheActorInfo / CacheActivationInfo`: 몽타주 콜백에서 `EndAbility`를 호출할 때 필요한 인자를 저장하기 위한 캐시.

### 2-2. `GA_Boss_Death.cpp`
| 함수 | 하는 일 |
| --- | --- |
| 생성자 | `Boss.Action.Death`(식별 태그) + `Boss.State.Dying`(다른 GA 차단용 owned tag) 등록 |
| `ActivateAbility` | `Super` 먼저 호출(=부모가 StartDeath). 그 후 AvatarActor 캐스팅 → AI Brain Stop → Movement Disable → DeathMontage 재생 |
| `OnDeathMontageCompleted` | 정상 종료 → `EndAbility(false)` (= bWasCancelled=false) |
| `OnDeathMontageCancelled` | 중단됨 → `EndAbility(true)` |

`EndAbility`는 부모가 자동으로 `FinishDeath()`를 부르므로 자식에서는 별도 처리 없이 호출만 하면 됨.

### 2-3. `BossCharacterBase.cpp`의 `BeginPlay`
서버에서만 동작. 두 가지를 함:
1. `BossAbilitySet->GiveToAbilitySystem(BossASC, nullptr)` — 보스가 사용할 GA들과 어트리뷰트 초기화 GE를 ASC에 등록.
2. `LyraHealthComponent::InitializeWithAbilitySystem(BossASC)` — `LyraCharacterWithAbilities`는 PawnExtComponent 흐름을 안 타서 자동 초기화가 안 일어남. 그래서 직접 호출해서 HealthSet과 OnOutOfHealth 바인딩을 세팅.

이 두 가지가 끝나면 죽음 흐름은 Lyra 기본 동작에 위임됨. 별도의 `OnDeathStarted` 바인딩이나 `TryActivateAbilitiesByTag` 같은 우회 처리는 필요 없음.

### 2-4. `BossCharacterBase::DebugKill`
디버그용. `LyraHealthComponent::DamageSelfDestruct(true)`를 호출 → `MaxHealth`만큼 데미지 GE를 자기 자신에게 적용 → HP 0 → 위 흐름이 그대로 발동.

---

## 3. 에디터에서 필요한 세팅 체크리스트

1. **`BossAbilitySet`(DataAsset) 의 `GrantedGameplayAbilities` 목록에 `GA_Boss_Death`(또는 그 BP 자식)가 들어있어야 함.**
   - 안 들어 있으면 트리거가 와도 활성화될 GA가 없어서 그대로 무한 대기.
2. `GA_Boss_Death`(또는 BP 자식) 에디터에서 `DeathMontage` 슬롯에 죽음 애니메이션 몽타주(`AM_BearDeath` 등) 지정.
3. `BossAbilitySet`이 보스 BP(`BP_TestBoss`)의 `BossAbilitySet` 프로퍼티에 할당되어 있어야 함.

> BP_GA_BossDeath를 별도로 만들었다면 그 BP의 부모 클래스를 `GA_Boss_Death`로 잡아야 위 흐름에 합류함. 부모가 그냥 `LyraGameplayAbility`이거나 다른 클래스면 똑같이 트리거를 못 받음.

---

## 4. 디버그 시 확인할 로그

호출 → 활성화까지 정상이면 다음 로그가 순서대로 찍힘.

```
[Death] DebugKill 호출 - Health=500.0 / Max=500.0 / DeathState=0 → DamageSelfDestruct
[Death] DamageSelfDestruct 직후 - Health=0.0 / DeathState=1 (1=DeathStarted)
[Death] GA_Boss_Death ActivateAbility 진입
[Death] DeathMontage 재생 시작: AM_BearDeath
```

증상별 진단:
- `DamageSelfDestruct 직후` 로그에서 DeathState가 계속 0 → **GameplayEvent.Death 트리거를 받는 GA가 없음**. AbilitySet에 GA_Boss_Death 등록 여부, BP 자식의 부모 클래스 확인.
- `GA_Boss_Death ActivateAbility 진입`은 찍히는데 몽타주는 안 나옴 → `DeathMontage` 미지정 또는 몽타주의 슬롯이 ABP에 없음.
- HP가 한 번에 안 떨어지고 조금씩만 깎임 → `DamageGameplayEffect_SetByCaller`(Lyra Default Game Data 자산) 또는 다른 GE가 SetByCaller 무시하고 고정값을 쓰는지 확인. `LogLyra Verbose`로 `HealthSet Damage Applied:` 로그 추적.

---

## 5. 왜 이 구조인가 (요약)

- **Lyra의 죽음 처리는 "이벤트 트리거 + GA + DeathState 머신"의 조합**으로 만들어져 있음. 
- `OnDeathStarted`/`OnDeathFinished`는 *결과 통지*이지 *트리거가 아님*. 그래서 캐릭터에서 `OnDeathStarted`를 듣고 GA를 직접 발동하려는 시도는 순환에 빠짐(StartDeath를 부르는 주체가 다름).
- `ULyraGameplayAbility_Death`를 상속해서 정식 흐름에 합류시키면, 캐릭터 쪽 코드가 깨끗해지고 Lyra가 보장하는 안전망(다른 GA 자동 취소, 블로킹 그룹 전환, FinishDeath 보장)을 그대로 받을 수 있음.

---

## 6. 관련 코드 위치

| 역할 | 경로 |
| --- | --- |
| 보스 죽음 GA (자식) | `Source/LyraGame/Boss/Abilities/GA_Boss_Death.h/.cpp` |
| Lyra 죽음 GA (부모) | `Source/LyraGame/AbilitySystem/Abilities/LyraGameplayAbility_Death.h/.cpp` |
| HP 0 → 이벤트 송출 | `Source/LyraGame/Character/LyraHealthComponent.cpp::HandleOutOfHealth` |
| OnOutOfHealth 발동 지점 | `Source/LyraGame/AbilitySystem/Attributes/LyraHealthSet.cpp::PostGameplayEffectExecute` |
| 보스 ASC/Health 초기화 | `Source/LyraGame/Boss/BossCharacterBase.cpp::BeginPlay` |
| 디버그 즉사 | `Source/LyraGame/Boss/BossCharacterBase.cpp::DebugKill` |
