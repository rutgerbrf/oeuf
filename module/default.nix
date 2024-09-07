flake: { lib, config, pkgs, ... }:
with lib;
let
  inherit (flake.packages.${pkgs.stdenv.hostPlatform.system}) oeuf-recvkv6;
  inherit (flake.packages.${pkgs.stdenv.hostPlatform.system}) oeuf-archiver;

  cfg = config.services.oeuf-recvkv6;
  archiverCfg = config.services.oeuf-archiver;
in
{
  options.services.oeuf-recvkv6 = {
    enable = mkEnableOption "oeuf-recvkv6";
    ndovProduction = mkEnableOption "usage of the NDOV Loket production ZeroMQ server";
    metricsAddr = mkOption {
      type = types.str;
    };
  };

  options.services.oeuf-archiver = with types; {
    enable = mkEnableOption "oeuf-archiver";
    s3 = mkOption {
      type = submodule {
        options = {
          accessKeyIDFile = mkOption {
            type = str;
          };
          secretAccessKeyFile = mkOption {
            type = str;
          };
          provider = mkOption {
            type = str;
          };
          region = mkOption {
            type = str;
          };
          endpoint = mkOption {
            type = str;
          };
          bucket = mkOption {
            type = str;
          };
        };
      };
    };
    prometheusPushURL = mkOption {
      type = str;
    };
    supplementaryServiceGroups = mkOption {
      type = listOf str;
    };
  };

  config = mkIf (cfg.enable || archiverCfg.enable) (mkMerge [
    {
      users.users.oeuf = {
        description = "oeuf service user";
        isSystemUser = true;
        group = "oeuf";
      };

      users.groups.oeuf = { };
    }
    (mkIf cfg.enable {
      systemd.services.oeuf-recvkv6 = {
        wants = [ "network-online.target" ];
        wantedBy = [ "multi-user.target" ];
        environment = {
          METRICS_ADDR = cfg.metricsAddr;
          NDOV_PRODUCTION = lib.boolToString cfg.ndovProduction;
        };
        serviceConfig = {
          User = config.users.users.oeuf.name;
          Group = config.users.users.oeuf.group;
          Restart = "always";
          StateDirectory = "oeuf";
          WorkingDirectory = "/var/lib/oeuf";
          ExecStart = "${lib.getBin oeuf-recvkv6}/bin/oeuf-recvkv6";
        };
      };
    })
    (mkIf archiverCfg.enable {
      systemd.timers.oeuf-archiver = {
        wantedBy = [ "timers.target" ];
        partOf = [ "oeuf-archiver.service" ];
        timerConfig = {
          OnBootSec = "5m";
          OnUnitActiveSec = "5m";
          Unit = "oeuf-archiver.service";
        };
      };

      systemd.services.oeuf-archiver = {
        wants = [ "network-online.target" ];
        environment = {
          S3_PROVIDER = archiverCfg.s3.provider;
          S3_REGION = archiverCfg.s3.region;
          S3_ENDPOINT = archiverCfg.s3.endpoint;
          S3_BUCKET = archiverCfg.s3.bucket;
          PROMETHEUS_PUSH_URL = archiverCfg.prometheusPushURL;
        };
        script = ''
          export S3_ACCESS_KEY_ID="$(cat ${archiverCfg.s3.accessKeyIDFile})"
          export S3_SECRET_ACCESS_KEY="$(cat ${archiverCfg.s3.secretAccessKeyFile})"
          ${lib.getBin oeuf-archiver}/bin/oeuf-archiver
        '';
        serviceConfig = {
          Type = "oneshot";
          User = config.users.users.oeuf.name;
          Group = config.users.users.oeuf.group;
          SupplementaryGroups = archiverCfg.supplementaryServiceGroups;
          StateDirectory = "oeuf";
          WorkingDirectory = "/var/lib/oeuf";
          AmbientCapabilities = "CAP_NET_BIND_SERVICE";
        };
      };
    })
  ]);
}
