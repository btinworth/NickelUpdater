# NickelUpdater

NickelUpdater keeps other Kobo plugins up to date by installing their latest GitHub releases directly on your Kobo.

It runs automatically when WiFi connects.

## Installation

1. Download `KoboRoot.tgz` from the [releases page](https://github.com/btinworth/NickelUpdater/releases/latest).
2. Copy `KoboRoot.tgz` into the `.kobo` folder on your Kobo.
3. Reboot your Kobo.

After reboot, NickelUpdater creates its config file in the `.adds/nickelupdater` folder.

## Configuration

Open the `.adds/nickelupdater/nickelupdater.conf` file in a text editor.

Add one plugin per line:

`owner/repo = installed_tag@installed_commit`

Keep `installed_tag@installed_commit` blank on first install. NickelUpdater fills it in after each successful update.

```conf
# NickelUpdater configuration
# Format: owner/repo = installed_tag@installed_commit
# Only include 'owner/repo =' on first install
btinworth/NickelUpdater =
```

Each `repo`'s latest release must include a `KoboRoot.tgz` asset, the same way NickelUpdater itself is installed.

## Uninstall

1. Create an empty file named `uninstall` in `.adds/nickelupdater`.
2. Reboot your Kobo.

## Credits

Built with [NickelHook](https://github.com/pgaskin/NickelHook).
