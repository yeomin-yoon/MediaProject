# Codex of Ashes

소울라이크 전투에 파밍과 반복 성장을 결합한 액션 RPG

**Unreal Engine 5.5** · Lyra Starter Game · Gameplay Ability System
장르 액션 RPG · 플랫폼 PC · UE 클라이언트 3인 · 개발 기간 9주 (2026.03 ~ 2026.05)
2026 AJOU SOFTCON 미디어프로젝트 출품

## 게임 소개

소울라이크 특유의 묵직한 액션과 도전적인 난이도 위에, 적을 처치해 얻은 아이템으로 빌드를
완성해가는 파밍 성장 구조를 결합했습니다. 획득한 아이템과 장착 상태는 로비 복귀와 맵 이동
이후에도 유지되어 다음 전투의 성장으로 이어집니다.

UE5의 Lyra Starter Game을 기반으로, 무기별 콤보와 공격 패턴, 치장 요소를 기존 구조를
크게 건드리지 않고 추가할 수 있도록 확장성을 고려해 설계했습니다.

## 주요 구현

**플레이어 · 전투 · 콤보** — [@byam12](https://github.com/byam12)

- 그래프 기반 콤보 에디터 — 공격 동작과 연결을 노드로 구성하고 분기별 입력키를 지정
- 락온, 공격 · 피격 처리, 부위별 데미지, 경직 · 넉다운 판정
- 무기별 액션 세트와 타격 부위에 따른 히트박스 판정

**보스** — [@ykd-yang](https://github.com/ykd-yang)

- GAS 기반 보스 행동 관리와 GameplayAbility 단위 공격 패턴 분리
- 데미지 · 상태이상을 GameplayEffect로만 적용해 공격 행동과 수치 변화를 분리
- GameplayTag 기반 상태 관리로 어빌리티 간 직접 참조 없이 조건 분기

**인벤토리 · 장비** — [@yeomin-yoon](https://github.com/yeomin-yoon)

- 대량 아이템 데이터를 위한 네트워크 복제 구조와 모듈형 아이템 설계
- 등급 · 랜덤 옵션으로 동일 장비도 서로 다른 능력치를 갖도록 구성, 장착 시 스탯 실시간 반영
- Drag & Drop UI와 획득 Toast, 맵 이동 · 재실행 이후 데이터를 유지하는 Persistence 구조

## 개발 환경

Unreal Engine 5.5 · C++ · Blueprint · Lyra Starter Game

## 관련 링크

공개용 저장소 — https://github.com/byam12/forpublic
