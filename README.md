# SOTN Ultimate 한국어화 (작업 중)

[SOTN Ultimate](https://github.com/renanbianchi/sotn-ultimate)에 한국어를 넣는 포크입니다. 아직 받을 수 있는 한국어판은 없습니다. 계획은 [ROADMAP.md](ROADMAP.md)에 있습니다.

A fork of SOTN Ultimate to add Korean. Nothing to download yet — see [ROADMAP.md](ROADMAP.md). The original mod is by renanbianchi; this fork is unofficial.

- 작업 브랜치: `ko` · 원작 추종 브랜치: `sotn-ultimate`
- 이 저장소에는 게임 데이터가 없습니다.

---

# SOTN Ultimate (PS1 Version)
# SOTN Ultimate (PS1 Version)

A mod of *Castlevania: Symphony of the Night* for the PlayStation that brings
the Sega Saturn version's exclusive content and the PSP *Dracula X Chronicles*
additions to the PS1, on one CD. **This repository contains no game data**:
players build the game from their own discs with the builder page on the
[Releases](https://github.com/renanbianchi/sotn-ultimate/releases) page.

The mod lives in [`mods/sotn-ultimate`](mods/sotn-ultimate); its
[README](mods/sotn-ultimate/README.md) covers the discs it takes, building
from source, the tests, how it sits on top of sotn-decomp (the developer
notes), the licence and the credits.

---

# About sotn-decomp (upstream)

The text below is sotn-decomp's own README.

# Castlevania: Symphony of the Night Decompilation

A work-in-progress decompilation of Castlevania Symphony of the Night for Sony PlayStation 1, Sony PlayStation Portable and Sega Saturn. It aims to recreate the source code from the existing binaries using static and/or dynamic analysis. The code compiles byte-for-byte to the same binaries of the game, effectively being a matching decompilation.

It currently supports the following versions of the game:

* `us` the reference build with the serial number SLUS-00067
* `hd` an unreleased PS1 Japanese build found in Castlevania: Dracula X Chronicles game data
* `pspeu` European build from Castlevania: Dracula X Chronicles
* `saturn` the port created by an external development team

This repo does not include any assets or assembly code necessary for compiling the binaries. A prior copy of the game is required to extract the required assets.

## Useful links

* [Build guide](https://github.com/Xeeynamo/sotn-decomp/wiki/Build)
* [Decompilation guide](https://github.com/Xeeynamo/sotn-decomp/wiki/Decompilation)
* [Progress report](https://sotn.xee.dev/)
* [decomp.dev progress](https://decomp.dev/Xeeynamo/sotn-decomp)

Decompiling a game is a mastodontic task. If you have some basic programming skills, please join us in this journey. Any contribution will be very appreciated!

[![Join to our Discord server](https://discord.com/api/guilds/1079389589950705684/widget.png?style=banner2)](https://sotn-discord.xee.dev/)

## Special thanks

This project is possible thanks to the hard work of tools provided by the Decompilation community:

* [mips2c](https://github.com/matt-kempster/m2c) from @matt-kempster to decompile MIPS assembly into C. This has proven to be more accurate than Hexrays IDA and Ghidra.
* [splat](https://github.com/ethteck/splat) from @ethteck to disassemble code and extract data with a symbol map. This tool provides the fundamentals of the SOTN decomp.
* [asm-differ](https://github.com/simonlindholm/asm-differ) from @simonlindholm to know how the decompiled code compares to the original binary.
* [decomp-permuter](https://github.com/simonlindholm/decomp-permuter) from @simonlindholm to pick different versions of the same code that better matches the original binary.
* [maspsx](https://github.com/mkst/maspsx) by @mkst to replicate the customized assembler used in the official PSX SDK.
* [decomp.me](https://github.com/decompme/decomp.me/) by @ethteck, @nanaian and @mkst to provide a collaborative decompilation site to share and contribute to work-in-progress decompiled functions.
* [frogress](https://github.com/decompals/frogress) by @ethteck to store and retrieve progression data.
* [esa-new](https://github.com/mkst/esa-new) by @mkst as an inspiration on how to set-up a PS1 decompilation project.
* [oot](https://github.com/zeldaret/oot) as an inspiration of what it is possible to achieve with a complete decompiled video game.
