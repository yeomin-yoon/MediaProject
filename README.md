# Codex of Ashes

던전 파밍과 누적 성장을 결합한 액션 게임

**Unreal Engine 5.5** · Lyra Starter Game · Gameplay Ability System<br>
장르 액션 · 플랫폼 PC · UE 클라이언트 3인 · 개발 기간 9주 (2026.03 ~ 2026.05)<br>
2026 AJOU SOFTCON 미디어프로젝트 출품

[시연 영상](https://youtu.be/r3E8gHR1bFk) · [상세 기술 문서](https://app.notion.com/p/Codex-of-Ashes-349e6578029f80efa8edc40cbe80b4f3)

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

- Lyra 인벤토리의 Fast Array · Item Instance 구조를 확장한 아이템 관리
- 등급 · 랜덤 옵션으로 동일 장비도 서로 다른 능력치를 갖도록 구성, 장착 시 스탯 실시간 반영
- Drag & Drop UI와 획득 Toast, 맵 이동 · 재실행 이후 데이터를 유지하는 Persistence 구조

## 개발 환경

Unreal Engine 5.5 · C++ · Blueprint · Lyra Starter Game

## 관련 링크

- [MediaProject](https://github.com/yeomin-yoon/MediaProject): 이 README와 프로젝트 코드를 확인할 수 있는 저장소입니다.
- [공개용 저장소](https://github.com/byam12/forpublic): 같은 프로젝트에서 외부 에셋과 비공개 자료 등을 제외해 별도로 정리한 공개본입니다. 인벤토리 코드만 분리한 저장소는 아닙니다.

공개용 저장소는 단독 실행용 전체 프로젝트가 아닙니다. 사용하려면 Unreal Engine 5.5.4의 Lyra Starter Game에 파일을 적용하고, 필요한 외부 에셋을 적법하게 확보해야 합니다. 포함·제외 파일과 사용 조건은 해당 저장소의 README 및 `LICENSE.md`를 확인해 주세요.

