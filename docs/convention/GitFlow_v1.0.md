![git-flow](../images/GitFlow_v1.0/git-flow.png)

# Git Branch 전략에 대해

## Git Branch 전략이란

**Git Branch 전략**: <span style="background-color: #7cffa4">여러 개발자가 하나의 저장소에서 작업을 할 때 협업을 효과적으로 하기 위해 Git Branch에 대한 **규칙을 정하고** 저장소를 **체계적으로 활용**하는 전략</span>

<br>

**Git Branch 전략**으로는 대표적으로 **Git-Flow 전략**, **GitHub-Flow 전략**으로 두 가지가 있다. 여기서는 Git-Flow 전략에 대해서만 알아보도록 할 것이다.

<br>

## Git Branch 전략의 중요성

여러 명의 개발자가 각자 다른 기능을 담당하는 브랜치를 사용하여 작업하면, 개발 중인 기능이나 수정사항이 <span style="background-color: #fff551">서로 독립적이게 되며 영향을 주지 않고 병렬적으로 진행</span>할 수 있다.

<br>

**[Git Branch 전략의 필요성]**

- 어떤 브랜치가 최신 브랜치인지 파악해야 함
- 어떤 브랜치를 Pull해서 개발을 시작해야 할지 명확히 해야 함
- 기능 개발 등의 활동을 할 때 각각 어떤 브랜치에 Push를 보내야 하는지 확실하게 정해야 함
- 핫픽스를 해야 하는데 어떤 브랜치를 기준으로 수정해야 하는지 정해야 함
- 배포 버전은 어떤 것을 골라야 하는지에 대해 명확한 기준이 필요함

<br>

***

# Git-Flow 전략

![git-flow-kr](../images/GitFlow_v1.0/git-flow-kr.png)

**Git-Flow 전략**은 <span style="background-color: #fff551">배포 안정성과 버전 관리 및 롤백 등 체계적인 운영이 가능</span>하지만 <span style="background-color: #fff551">더 많은 제어와 복잡성을 가지고 있어 특정 기능이나 수정을 빠르게 배포해야 할 경우 등에서 유연성이 다소 떨어진다.</span>

<br>

**[브랜치 종류]**

1. `main`: 기능 개발, QA가 완료되어 라이브 서버에 제품으로 **출시**되는 브랜치
2. `develop`: 다음 출시 버전에 대비하여 **개발**하는 브랜치
3. `feature`: **새로운 기능**을 개발하는 브랜치
4. `release`: feature 브랜치에서 개발이 완료된 기능에 대해 **QA를 수행**하여 다음 버전 출시를 준비하는 브랜치
5. `hotfix`: main 브랜치에서 발생한 **버그를 수정**하는 브랜치

<br>

## 메인 브랜치

메인 브랜치는 <span style="color: #FF0000">main 브랜치</span>와 <span style="color: #0000ff">develop 브랜치</span>로 두 브랜치가 있다.

<br>

- <span style="color: #FF0000">main 브랜치</span>: <span style="background-color: #fff551">배포 가능한 상태</span>만을 관리하는 브랜치

- <span style="color: #0000ff">develop 브랜치</span>: 다음에 <span style="background-color: #fff551">배포할 것을 개발</span>하는 브랜치

<br>

<span style="color: #0000ff">develop 브랜치</span>는 통합 브랜치의 역할을 하며, <span style="background-color: #fff551">평소에는 <span style="color: #0000ff">develop 브랜치</span>를 기반으로 개발을 진행</span>한다.

<br>

## 서브 브랜치

서브 브랜치는 <span style="color: #00B900">feature 브랜치</span>, <span style="color: #00c1c1">release 브랜치</span>, <span style="color: #9f339f">hotfix 브랜치</span>가 있다.

<br>

### feature 브랜치

- <span style="background-color: #fff551">새로운 기능을 추가</span>한다.
- **브랜치 네이밍**: <span style="color: #00B900">feature-1</span>, <span style="color: #00B900">feature-2</span>와 같이 <span style="background-color: #fff551"><span style="color: #00B900">feature-*</span></span>형식으로 한다.

<br>

<span style="color: #0000ff">develop 브랜치</span>에는 기존에 잘 작동하는 개발코드가 담겨있으며, <span style="color: #00B900">feature 브랜치</span>는 새로 변경될 개발코드를 분리하고 각각 보존하는 역할을 한다.

즉, <span style="color: #00B900">feature 브랜치</span>는 기능을 다 완성할 때까지 유지하고, 기능이 완성되면 <span style="color: #0000ff">develop 브랜치</span>로 merge하고 결과가 좋지 못하면 버린다.

<br>

### release 브랜치

- 완성된 새로운 기능들을 QA함으로써 다음 버전을 <span style="color: #FF0000">main 브랜치</span>로 배포할 준비를 한다.
- **브랜치 네이밍**: <span style="color: #00c1c1">release</span>

<br>

<span style="color: #00B900">feature 브랜치</span>로부터 새로운 기능이 <span style="color: #0000ff">develop 브랜치</span>로 merge 되었다면 QA를 수행하기 위해 <span style="color: #0000ff">develop 브랜치</span>로부터 <span style="color: #00c1c1">release 브랜치</span>를 생성한다.

배포 가능한 상태가 되면 <span style="color: #00c1c1">release 브랜치</span>를 <span style="color: #FF0000">main 브랜치</span>로 merge 시키고 출시된 <span style="color: #FF0000">main 브랜치</span>에 버전 태그(e.g. ver 1.1.0, ver 1.3.1)를 추가한다.

<span style="color: #FF0000">main 브랜치</span>로 배포 완료 후 <span style="color: #0000ff">develop 브랜치</span>로도 merge 작업을 수행해야 한다.

<br>

### hotfix 브랜치

- 라이브 서버에 출시된 제품(<span style="color: #FF0000">main 브랜치</span>)에서 버그가 발생했을 경우에 <span style="color: #FF0000">main 브랜치</span>에서 <span style="color: #9f339f">hotfix 브랜치</span>를 생성하고 <span style="color: #9f339f">hotfix 브랜치</span>에서 버그 수정 작업을 하며 버그 수정이 완료되면 <span style="color: #FF0000">main 브랜치</span>, <span style="color: #0000ff">develop 브랜치</span>에 곧장 반영해주며 tag를 통해 버그 수정 정보를 기록해둔다.
- **브랜치 네이밍**: <span style="color: #9f339f">hotfix-1</span>, <span style="color: #9f339f">hotfix-2</span>와 같이 <span style="background-color: #fff551"><span style="color: #9f339f">hotfix-*</span></span>형식으로 한다.

<br>

버그를 수정하는 동안에도 다른 개발자들은 <span style="color: #0000ff">develop 브랜치</span>에서 하던 일을 계속할 수 있다.

<span style="color: #00c1c1">release 브랜치</span>가 생성되어 관리되고 있는 상태라면 <span style="color: #00c1c1">release 브랜치</span>에도 hotfix 정보를 merge 시켜 다음 버전 배포 시 반영이 정상적으로 이루어질 수 있도록 한다.

<span style="color: #9f339f">hotfix 브랜치</span>는 일반적으로 긴급하게 버그를 수정하기 위해 생성되는 브랜치이기 때문에 버그 수정을 완료하면 제거하는 일회성 브랜치이다.

<br>

## Git-Flow 흐름

1. 신규 기능 개발을 위해 개발자는  <span style="color: #0000ff">develop 브랜치</span>에서 <span style="color: #00B900">feature 브랜치</span>를 생성하여 작업을 진행한다.
2. 신규 기능 개발 작업이 완료된 <span style="color: #00B900">feature 브랜치</span>를 <span style="color: #0000ff">develop 브랜치</span>로 merge한다. (일반적으로 프로젝트 진행 시에는 Pull Request를 통해 작업 내용을 Review받은 후 해당 PR(Pull Request)을 merge하는 방식으로 진행된다.)
3. 다음 버전 출시를 위해 개발 중인 <span style="color: #0000ff">develop 브랜치</span>에서 <span style="color: #00c1c1">release 브랜치</span>를 생성하여 QA를 수행함으로써 배포를 위한 준비를 한다. 이 때 QA를 수행하면서 발견되는 버그들은 <span style="color: #00c1c1">release 브랜치</span>에서 바로 수정한다.
4. 충분한 QA 후에 <span style="color: #FF0000">main 브랜치</span>로 merge하여 라이브 서버로 출시한다.
5. 상용 배포 이후 <span style="color: #00c1c1">release 브랜치</span>에서 미처 발견하지 못한 새로운 버그들은 <span style="color: #FF0000">main 브랜치</span>에서 <span style="color: #9f339f">hotfix 브랜치</span>를 생성하여 <span style="color: #9f339f">hotfix 브랜치</span>에서 버그 수정 작업을 한다.