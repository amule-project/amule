# aMule

![aMule](https://raw.githubusercontent.com/amule-org/amule/master/org.amule.aMule.png)

aMule là một client tương tự eMule dành cho mạng eDonkey và Kademlia.

[Diễn đàn] | [Tài liệu] | [FAQ]

[Diễn đàn]:   https://github.com/amule-org/amule/discussions "aMule Forum"
[Tài liệu]:   https://amule-org.github.io/docs "aMule Documentation"
[FAQ]:         https://amule-org.github.io/docs/manual/faq "FAQ on aMule"

## Thông báo quan trọng

Dự án aMule sẽ tiếp tục được phát triển tại [aMule-org repo](https://github.com/amule-org/amule) mới. Lý do chúng tôi phải tạo tổ chức mới là vì Gonosztopi, người sở hữu duy nhất của [aMule Project](https://github.com/amule-project), đã mất liên lạc. Hệ quả là trong tổ chức **aMule-project**, chúng tôi không còn khả năng cập nhật cơ sở hạ tầng theo nhu cầu của dự án.

## Tổng quan

aMule là một client đa nền tảng cho mạng chia sẻ tập tin eD2k / Kad,
ban đầu là một fork của client Windows eMule (thông qua xMule và lMule).
aMule được khởi động vào tháng 8 năm 2003.

Các nền tảng được hỗ trợ hiện nay: Linux, FreeBSD, OpenBSD, macOS và Windows
(MSYS2 / mingw-w64), trên cả x86_64 và ARM64.

aMule hướng đến việc giữ giao diện và trải nghiệm gần giống eMule để người dùng
chuyển đổi giữa hai ứng dụng không gặp nhiều khó khăn. Các tính năng mới ở cấp độ
giao thức eMule thường được tích hợp vào aMule ngay sau đó.

---

| Bản phân phối |
| --- |
| [![Arch Linux](https://repology.org/badge/version-for-repo/arch/amule.svg)](https://archlinux.org/packages/extra/x86_64/amule/) |
| [![AUR](https://repology.org/badge/version-for-repo/aur/amule.svg)](https://aur.archlinux.org/packages/amule) |
| [![Debian stable](https://badges.debian.net/badges/debian/stable/amule/version.svg)](https://buildd.debian.org/amule) |
| [![Debian testing](https://badges.debian.net/badges/debian/testing/amule/version.svg)](https://buildd.debian.org/amule) |
| [![FreeBSD](https://repology.org/badge/version-for-repo/freebsd/amule.svg)](https://www.freshports.org/net-p2p/amule/) |
| [![Gentoo](https://repology.org/badge/version-for-repo/gentoo/amule.svg)](https://packages.gentoo.org/packages/net-p2p/amule) |
| [![Kali Linux](https://repology.org/badge/version-for-repo/kali_rolling/amule.svg)](https://pkg.kali.org/pkg/amule) |
| [![Manjaro](https://repology.org/badge/version-for-repo/manjaro_stable/amule.svg)](https://repology.org/project/amule/versions) |
| [![NixOS 25.05](https://repology.org/badge/version-for-repo/nix_stable_25_05/amule.svg)](https://search.nixos.org/packages?channel=25.05&query=amule) |
| [![OpenBSD](https://repology.org/badge/version-for-repo/openbsd/amule.svg)](https://openports.pl/path/net/amule) |
| [![openSUSE Tumbleweed (Packman)](https://repology.org/badge/version-for-repo/packman_opensuse_tumbleweed/amule.svg)](http://packman.links2linux.org/package/aMule) |
| [![RPMFusion Fedora 42](https://repology.org/badge/version-for-repo/rpmfusion_fedora_42/amule.svg)](https://repology.org/project/amule/versions) |
| [![Slackware](https://repology.org/badge/version-for-repo/slackbuilds/amule.svg)](https://slackbuilds.org/result/?search=amule) |
| [![Solus](https://repology.org/badge/version-for-repo/solus/amule.svg)](https://repology.org/project/amule/versions) |
| [![Ubuntu 24.04 LTS](https://repology.org/badge/version-for-repo/ubuntu_24_04/amule.svg)](https://packages.ubuntu.com/noble/amule) |
| [![Ubuntu 25.04](https://repology.org/badge/version-for-repo/ubuntu_25_04/amule.svg)](https://packages.ubuntu.com/plucky/amule) |

---

Thống kê phát triển:

| [![Open Issues](https://img.shields.io/github/issues/amule-project/amule)](https://github.com/amule-project/amule/issues) | [![Open Pull Requests](https://img.shields.io/github/issues-pr/amule-project/amule)](https://github.com/amule-project/amule/pulls) |
| --- | --- |
| [![Bug](https://img.shields.io/github/issues/amule-project/amule/bug)](https://github.com/amule-project/amule/issues?q=is%3Aopen+is%3Aissue+label%3Abug) | |
| [![Bug - Delayed Fix](https://img.shields.io/github/issues/amule-project/amule/bug%20-%20delayed%20fix)](https://github.com/amule-project/amule/issues?labels=bug%20-%20delayed+fix) | |
| [![Feature Request](https://img.shields.io/github/issues/amule-project/amule/feature%20request)](https://github.com/amule-project/amule/issues?labels=feature+request) | |
| [![Enhancement](https://img.shields.io/github/issues/amule-project/amule/enhancement)](https://github.com/amule-project/amule/issues?labels=enhancement) | |

## Tính năng

* `amule` — client GUI tích hợp tất cả trong một.
* `amuled` — daemon chạy ngầm, không có GUI.
* `amulegui` — GUI từ xa; kết nối với `amuled` cục bộ hoặc từ xa qua giao thức
  EC (External Connection).
* `amuleweb` — giao diện HTTP cho `amuled` đang chạy.
* `amulecmd` — CLI tương tác cho `amuled` đang chạy.

## Biên dịch

aMule sử dụng CMake. Bắt đầu nhanh:

```sh
cmake -B build -DBUILD_MONOLITHIC=YES -DBUILD_REMOTEGUI=YES
cmake --build build -j"$(nproc)"
sudo cmake --install build
```

Xem [docs/INSTALL.md](docs/INSTALL.md) để biết danh sách đầy đủ các dependencies,
các tùy chọn build (`BUILD_DAEMON`, `BUILD_AMULECMD`, `ENABLE_NLS`, `ENABLE_UPNP`,
`ENABLE_IP2COUNTRY`, v.v.) và các lưu ý theo từng nền tảng. Workflow CI
[`.github/workflows/ccpp.yml`](.github/workflows/ccpp.yml) là tài liệu tham chiếu
chính xác nhất về các deps và flags được dùng để build aMule trên Linux, macOS và Windows.

## Cài đặt ban đầu

aMule đi kèm với các cài đặt mặc định hợp lý và có thể sử dụng ngay. Tuy nhiên,
để nhận được [HighID] bạn cần mở các cổng của aMule trên tường lửa và/hoặc
chuyển tiếp chúng trên router. Xem [hướng dẫn kết nối mạng][network] để biết thêm chi tiết.

[HighID]:  https://amule-org.github.io/docs/p2p-networks/ed2k/high-id "What is LowID and HighID?"
[network]: https://amule-org.github.io/docs/manual/configuration/network-connectivity "Network connectivity"

## Báo cáo lỗi

Nếu bạn tìm thấy lỗi hoặc muốn đề xuất tính năng, vui lòng mở issue trên
[GitHub][5] (ưu tiên) hoặc báo cáo trên [diễn đàn]. Một báo cáo lỗi tốt
bao gồm phiên bản aMule chính xác (`amuled --version`), nền tảng bạn đang dùng
và các bước để tái hiện lỗi. Xem [hướng dẫn báo cáo lỗi][bug-report] để biết
hướng dẫn chi tiết về cách đính kèm backtrace và các bước tái hiện.

[5]:          https://github.com/amule-org/amule/issues "aMule Issues"
[bug-report]: https://amule-org.github.io/docs/contributing/bug-report "Bug Report Instructions"

## Đóng góp

*Mọi đóng góp đều được chào đón!*

Xem [hướng dẫn đóng góp][contributing] để biết cách tham gia. Tóm tắt:

* **Code** — sửa lỗi, triển khai tính năng, cải thiện hiệu năng. Cách được ưu tiên
  là tạo [pull request][6] trên GitHub; các bản patch trên [diễn đàn] cũng được chấp nhận.
* **Dịch thuật** — dịch aMule, tài liệu hoặc website của dự án sang ngôn ngữ của bạn.
* **Tài liệu** — giúp cải thiện tài liệu dự án tại
  [amule-org.github.io/docs][Tài liệu].

[6]:            https://github.com/amule-org/amule/pulls "aMule Pull Requests"
[contributing]: https://amule-org.github.io/docs/contributing "Contributing to aMule"
