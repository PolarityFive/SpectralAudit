#pragma once

namespace CONSTANTS {
    constexpr const char* INPUT_DIRECTORY = R"(T:\Music\Pop\Lady Gaga - The Fame)";
    constexpr const char* DB_PATH = R"(Q:\\Visual Studio Projects\\Sqlite\\spectral_audit.db)";
    constexpr const char* DB_PATH_V2 = R"(Q:\\Visual Studio Projects\\Sqlite\\spectral_audit_V0.2.db)";
    constexpr const char* DB_PATH_V3 = R"(Q:\\Visual Studio Projects\\Sqlite\\spectral_audit_V0.3.db)";
    constexpr int WINDOW_SIZE = 2048;
    constexpr int HOP_SIZE = 256;
}

namespace CONSTANTS_TEST {
    constexpr const char* TEST_DIRECTORY = R"(Q:\Visual Studio Projects\SpectralAudit\Doom Metal Test Library)";
    constexpr const char* TEST_WEIRD_DIR = R"(Q:\Visual Studio Projects\SpectralAudit\Doom Metal Test Library\October Tide - The Cancer Pledge)";
}