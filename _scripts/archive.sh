#!/bin/bash
#
# archive.sh — Build, sign, notarize, and (optionally) DMG-package
# the Icons app for Developer ID distribution outside the Mac App Store.
#
# Usage:
#   ./_scripts/archive.sh                          # build + export signed .app
#   ./_scripts/archive.sh +                        # bump build, then build
#   ./_scripts/archive.sh + --notarize             # also submit to Apple notary
#   ./_scripts/archive.sh + --notarize --dmg       # also produce a .dmg
#
# One-time notarization setup:
#   xcrun notarytool store-credentials icons-notary \
#       --apple-id piecuch.pawel@gmail.com \
#       --team-id NR5F9UD6RP \
#       --password APP_SPECIFIC_PASSWORD
#
# (Override the keychain profile name with NOTARY_PROFILE=…)
# (Override the Qt bin dir with QT_BIN=…)
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
DIST_DIR="$PROJECT_DIR/dist"
PRO_FILE="$PROJECT_DIR/Icons.pro"
APP_NAME="Icons"
BUNDLE_ID="com.komsoft.icons"
TEAM_ID="NR5F9UD6RP"
ENTITLEMENTS="$PROJECT_DIR/Resources/${APP_NAME}.entitlements"
QT_BIN="${QT_BIN:-/Users/piecuchp/Qt/6.8-static/clang_64/bin}"
QMAKE="$QT_BIN/qmake"
NOTARY_PROFILE="${NOTARY_PROFILE:-icons-notary}"

# Parse Icons.pro
VERSION=$(awk -F '=' '/^VERSION[[:space:]]*=/ { gsub(/[[:space:]]/, "", $2); print $2; exit }' "$PRO_FILE")
BUILD_NUM=$(awk -F '=' '/^BUILD[[:space:]]*=/ { gsub(/[[:space:]]/, "", $2); print $2; exit }' "$PRO_FILE")

# Args
DO_NOTARIZE=false
DO_DMG=false
BUMP_BUILD=false
while [[ $# -gt 0 ]]; do
    case "$1" in
        +)          BUMP_BUILD=true;  shift ;;
        --notarize) DO_NOTARIZE=true; shift ;;
        --dmg)      DO_DMG=true;      shift ;;
        -h|--help)
            sed -n '/^# Usage:/,/^$/p' "$0" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [+] [--notarize] [--dmg]"
            exit 1 ;;
    esac
done

if [ "$BUMP_BUILD" = true ]; then
    BUILD_NUM=$((BUILD_NUM + 1))
    sed -i '' -E "s/^BUILD[[:space:]]*=.*/BUILD = $BUILD_NUM/" "$PRO_FILE"
    echo "Build number incremented to $BUILD_NUM"
fi

echo "=== Icons Developer ID Builder ==="
echo "Version:   $VERSION (build $BUILD_NUM)"
echo "Bundle ID: $BUNDLE_ID"
echo "Team ID:   $TEAM_ID"
echo "Qt:        $QT_BIN"
echo ""

if [ ! -x "$QMAKE" ]; then
    echo "ERROR: qmake not found at $QMAKE"
    echo "Set QT_BIN=… to point at your Qt 6.x bin dir."
    exit 1
fi

# --- Find Developer ID Application identity ---
echo "--- Checking signing identity ---"
IDENTITY=$(security find-identity -v -p codesigning \
    | grep "Developer ID Application" | head -1 \
    | sed -E 's/.*"([^"]+)".*/\1/' || true)
if [ -z "$IDENTITY" ]; then
    echo "ERROR: No 'Developer ID Application' certificate found in Keychain."
    echo ""
    echo "Available identities:"
    security find-identity -v -p codesigning
    echo ""
    echo "Create one of type 'Developer ID Application' at:"
    echo "  https://developer.apple.com/account/resources/certificates"
    exit 1
fi
echo "Using: $IDENTITY"
echo ""

# Strip quarantine flags so codesign doesn't complain about source files
echo "--- Stripping quarantine attributes ---"
xattr -dr com.apple.quarantine "$PROJECT_DIR" 2>/dev/null || true
echo ""

# --- Build with qmake + make ---
BUILD_DIR="$PROJECT_DIR/build-macx-clang"
APP_BUILT="$BUILD_DIR/${APP_NAME}.app"

echo "--- Cleaning previous build ---"
rm -rf "$APP_BUILT"
[ -f "$PROJECT_DIR/Makefile" ] && (cd "$PROJECT_DIR" && make distclean >/dev/null 2>&1 || true)

echo "--- Running qmake ---"
(cd "$PROJECT_DIR" && "$QMAKE" -spec macx-clang CONFIG+=release CONFIG-=debug "$PRO_FILE")

echo "--- Building (make -j$(sysctl -n hw.ncpu)) ---"
(cd "$PROJECT_DIR" && make -j"$(sysctl -n hw.ncpu)" 2>&1 | tail -20)

if [ ! -d "$APP_BUILT" ]; then
    echo "ERROR: build did not produce $APP_BUILT"
    exit 1
fi
echo ""

# --- Patch Info.plist with version/build, bundle id, hi-res support ---
PLIST="$APP_BUILT/Contents/Info.plist"
echo "--- Patching Info.plist ---"
/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier $BUNDLE_ID" "$PLIST" 2>/dev/null \
    || /usr/libexec/PlistBuddy -c "Add :CFBundleIdentifier string $BUNDLE_ID" "$PLIST"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $VERSION" "$PLIST" 2>/dev/null \
    || /usr/libexec/PlistBuddy -c "Add :CFBundleShortVersionString string $VERSION" "$PLIST"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $BUILD_NUM" "$PLIST" 2>/dev/null \
    || /usr/libexec/PlistBuddy -c "Add :CFBundleVersion string $BUILD_NUM" "$PLIST"
/usr/libexec/PlistBuddy -c "Set :NSHighResolutionCapable true" "$PLIST" 2>/dev/null \
    || /usr/libexec/PlistBuddy -c "Add :NSHighResolutionCapable bool true" "$PLIST"
/usr/libexec/PlistBuddy -c "Set :LSApplicationCategoryType public.app-category.graphics-design" "$PLIST" 2>/dev/null \
    || /usr/libexec/PlistBuddy -c "Add :LSApplicationCategoryType string public.app-category.graphics-design" "$PLIST"
echo ""

# --- Stage signed copy in dist/ ---
mkdir -p "$DIST_DIR"
EXPORT_DIR="$DIST_DIR/${APP_NAME}-${VERSION}-${BUILD_NUM}"
APP_DIST="$EXPORT_DIR/${APP_NAME}.app"
echo "--- Staging app to $EXPORT_DIR ---"
rm -rf "$EXPORT_DIR"
mkdir -p "$EXPORT_DIR"
/usr/bin/ditto "$APP_BUILT" "$APP_DIST"
echo ""

# --- Sign with hardened runtime + secure timestamp ---
echo "--- Signing with Developer ID + hardened runtime ---"
codesign --force --deep \
    --sign "$IDENTITY" \
    --timestamp \
    --options runtime \
    --entitlements "$ENTITLEMENTS" \
    "$APP_DIST"
echo ""

# Quick sanity: verify the signature and that hardened runtime is on
echo "--- Verifying signature ---"
codesign --verify --deep --strict --verbose=2 "$APP_DIST" 2>&1 | tail -5
codesign -d --entitlements - --xml "$APP_DIST" >/dev/null 2>&1 \
    && echo "Entitlements: ok"
codesign -d --verbose=2 "$APP_DIST" 2>&1 | grep -E "flags=|Identifier=|Authority=" | head -5
echo ""

# --- Notarize ---
if [ "$DO_NOTARIZE" = true ]; then
    NOTARIZE_ZIP="$DIST_DIR/${APP_NAME}-${VERSION}-${BUILD_NUM}.zip"
    echo "--- Submitting .app to Apple notary (profile: $NOTARY_PROFILE) ---"
    /usr/bin/ditto -c -k --keepParent "$APP_DIST" "$NOTARIZE_ZIP"
    if ! xcrun notarytool submit "$NOTARIZE_ZIP" \
            --keychain-profile "$NOTARY_PROFILE" \
            --wait; then
        echo ""
        echo "Notarization failed. To inspect the most recent submission:"
        echo "  xcrun notarytool history --keychain-profile $NOTARY_PROFILE"
        echo "  xcrun notarytool log <submission-id> --keychain-profile $NOTARY_PROFILE"
        exit 1
    fi
    echo ""
    echo "--- Stapling notarization ticket ---"
    xcrun stapler staple "$APP_DIST"
    xcrun stapler validate "$APP_DIST"
    rm -f "$NOTARIZE_ZIP"
    echo ""
fi

# --- Optional DMG ---
if [ "$DO_DMG" = true ]; then
    DMG_PATH="$DIST_DIR/${APP_NAME}-${VERSION}-${BUILD_NUM}.dmg"
    echo "--- Building DMG ---"
    rm -f "$DMG_PATH"
    hdiutil create \
        -volname "${APP_NAME} ${VERSION}" \
        -srcfolder "$APP_DIST" \
        -ov -format UDZO \
        "$DMG_PATH" >/dev/null
    codesign --sign "$IDENTITY" --timestamp "$DMG_PATH"
    if [ "$DO_NOTARIZE" = true ]; then
        echo "--- Notarizing DMG ---"
        if ! xcrun notarytool submit "$DMG_PATH" \
                --keychain-profile "$NOTARY_PROFILE" \
                --wait; then
            echo "DMG notarization failed."
            exit 1
        fi
        xcrun stapler staple "$DMG_PATH"
        xcrun stapler validate "$DMG_PATH"
    fi
    echo "DMG: $DMG_PATH"
    echo ""
fi

echo "=== Done ==="
echo "App: $APP_DIST"
[ "$DO_DMG" = true ] && echo "DMG: $DIST_DIR/${APP_NAME}-${VERSION}-${BUILD_NUM}.dmg"
if [ "$DO_NOTARIZE" = false ]; then
    echo ""
    echo "(not notarized — Gatekeeper will warn end users; re-run with --notarize)"
fi
