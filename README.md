# INPUT_ECU

입력 신호 처리를 담당하는 ECU(Electronic Control Unit) 펌웨어 프로젝트입니다. Infineon TC275 TriCore 마이크로컨트롤러 기반으로 ADC, 버튼, CAN 통신 등의 입력 신호를 처리합니다.

## 개요

- **마이크로컨트롤러**: Infineon TC275 (TriCore)
- **프로세서**: 3개의 CPU 코어 (Cpu0, Cpu1, Cpu2)
- **컴파일러**: TASKING
- **프레임워크**: Infineon iLLD (Embedded Application Software Framework)

## 주요 기능

- **ADC 입력 처리**: 아날로그 센서 신호 수집 및 변환
- **버튼 입력**: 디지털 입력 신호 처리
- **CAN 통신**: CAN 버스를 통한 신호 전송

## 프로젝트 구조

### 메인 애플리케이션

- **App_InputEcu.c/h**: 입력 ECU 애플리케이션 계층
- **If_InputEcu.c/h**: 입력 ECU 인터페이스 정의
- **Can_TxInput.c/h**: CAN 메시지 전송 모듈

### 드라이버 계층

- **Drv_AdcInput.c/h**: ADC 입력 드라이버
- **Drv_Button.c/h**: 버튼 입력 드라이버

### CPU 메인

- **Cpu0_Main.c**: CPU0 메인 진입점

### 설정 및 라이브러리

- **Configurations/**: 프로젝트 설정 파일
  - `Ifx_Cfg.h`: Infineon iLLD 설정
  - `Debug/`: 디버그 설정
- **Libraries/**: 외부 라이브러리
  - `iLLD/`: Infineon Low-Level Drivers
  - `Infra/`: 인프라 계층
  - `Service/`: 서비스 계층

### 링커 스크립트

- **Lcf_Gnuc_Tricore_Tc.lsl**: GNU 컴파일러용 링커 스크립트
- **Lcf_Tasking_Tricore_Tc.lsl**: TASKING 컴파일러용 링커 스크립트

## 빌드 방법

### 필수 환경

- TASKING TriCore EDE (또는 HIGHTEC IDE)
- Infineon iLLD TC27D 라이브러리
- GCC 또는 TASKING 컴파일러

### 빌드 단계

1. 프로젝트를 TASKING IDE에서 개방
2. `tc275_input_ecu TriCore Debug (TASKING).launch` 설정 선택
3. 프로젝트 클린 및 빌드
   ```
   Clean Project
   Build Project
   ```

## 디버깅

- **디버그 설정**: `tc275_input_ecu TriCore Debug (TASKING).launch`
- **방법**: TASKING IDE의 디버거를 사용하여 실시간 디버깅 가능
- **핀 설정**: `Libraries/pinmapper.pincfg` 파일로 핀 할당 관리

## 핵심 모듈

### ADC 입력 (Drv_AdcInput)

아날로그 센서 입력을 디지털 값으로 변환합니다.

### 버튼 입력 (Drv_Button)

디지털 입력 신호를 감지하고 처리합니다.

### CAN 통신 (Can_TxInput)

변환된 입력 데이터를 CAN 메시지로 포장하여 전송합니다.

## 기술 사양

- **통신 프로토콜**: CAN
- **입력 유형**: ADC (아날로그), 디지털 (버튼, GPIO)
- **컴파일 타겟**: TriCore TC275

## 참고 사항

- OneEye 프로파일링 도구 설정 가능 (`tc275_input_ecu.OneEye`)
- 핀 설정 변경 시 `pinmapper.pincfg` 파일 수정 후 재생성
- iLLD 라이브러리는 Infineon 공식 배포판 사용

## 라이선스

Boost Software License - Version 1.0 (자세한 내용은 소스 파일의 헤더 참고)

## 관련 문서

- Infineon TC275 데이터시트
- iLLD 문서: `Libraries/iLLD/` 참고
