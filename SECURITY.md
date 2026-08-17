# Security Policy

## Reporting a vulnerability

Please report security vulnerabilities privately through the repository’s GitHub security-reporting channel when available. Include the affected revision, a minimal reproduction, impact, and any relevant evidence. Do not include credentials, private network identifiers, proprietary files, or unsafe machine commands.

OpenXHC is research software and is not authorized for machine control. A protocol or software defect must not be investigated by issuing unvalidated writes to installed machinery.

## Safety-sensitive reports

Do not test a suspected defect on an enabled machine. Reproduce it first with the simulator or an electrically isolated controller. Reports must state whether motors, drive enable, spindle/VFD control, tooling, and workholding were physically disconnected.

## Supported versions

No production version is supported. The `main` branch is research documentation only until a release states an evidence level and safety scope explicitly.
