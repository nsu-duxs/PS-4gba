#include "i18n.h"
#include "ps4_jailbreak.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define PSGBA_CONFIG_MAGIC   0x41424750U /* "PGBA" */
#define PSGBA_CONFIG_VERSION 1U

typedef struct {
    uint32_t magic;
    uint32_t version;
    int32_t  language;
    uint32_t reserved;
} PS4GBA_ConfigData;

AppLanguage g_current_language = LANG_PT_BR;

const char* g_strings[LANG_COUNT][STR_STRING_COUNT] = {
    /* ======================================================================
     * LANG_PT_BR - Portugues (Brasil / Portugal)
     * ====================================================================== */
    [LANG_PT_BR] = {
        [STR_APP_TITLE]              = "PSgba - GBA 4-WAY SPLIT-SCREEN EMULATOR v00.10",
        [STR_PRESS_ANY_BUTTON]       = "Aperte qualquer botao para comecar",
        [STR_MENU_RESUME]            = "Continuar Jogo",
        [STR_MENU_RESET]             = "Voltar ao Launcher (Fechar ROM)",
        [STR_MENU_SAVES]             = "[ JOGO & SAVES ]",
        [STR_MENU_CHEATS]            = "[ TRAPACAS / CHEATS ]",
        [STR_MENU_SETTINGS]          = "[ VIDEO & AUDIO ]",
        [STR_MENU_EXIT]              = "Sair do Emulador (Encerrar Aplicativo)",
        [STR_SETTING_LANGUAGE]       = "Idioma / Language: < [Portugues] / English >",
        [STR_SETTING_AUDIO_SFX]      = "Sons do Menu: < %s >",
        [STR_SETTING_VIDEO_MODE]     = "Proporcao da Tela: < %s >",
        [STR_DISCLAIMER_TITLE]       = "[ AVISO LEGAL & COMPROMISSO ]",
        [STR_DISCLAIMER_BODY]        = "Este projeto foi desenvolvido estritamente para fins de preservacao historica e engenharia de software.",
        [STR_DISCLAIMER_ACCEPT]      = "[X] Concordo e Desejo Prosseguir",

        /* Splash Screen & Disclaimer */
        [STR_SPLASH_CONNECTING]      = "CONECTANDO 4 JOGADORES...",
        [STR_SPLASH_SUBTITLE]        = "4-WAY SPLIT-SCREEN EMULATOR v00.10",
        [STR_SPLASH_DEVELOPED_BY]    = "DESENVOLVIDO POR",
        [STR_SPLASH_SKIP]            = "[X / OPTIONS] Pular",
        [STR_DISCLAIMER_P1_L1]       = "Este projeto foi desenvolvido estritamente para fins de",
        [STR_DISCLAIMER_P1_L2]       = "preservacao historica e engenharia de software.",
        [STR_DISCLAIMER_ALERT]       = "Somos COMPLETAMENTE CONTRA A PIRATARIA em qualquer forma.",
        [STR_DISCLAIMER_INTRO]       = "Ao utilizar o PSgba, voce declara e concorda que:",
        [STR_DISCLAIMER_ITEM1_L1]    = "Nao utilizara pirataria para obter quaisquer ROMs",
        [STR_DISCLAIMER_ITEM1_L2]    = "ou arquivos de BIOS.",
        [STR_DISCLAIMER_ITEM2_L1]    = "Utilizara exclusivamente copias de seguranca (backups) geradas a",
        [STR_DISCLAIMER_ITEM2_L2]    = "partir dos seus proprios cartuchos originais legalmente adquiridos.",
        [STR_DISCLAIMER_ITEM3_L1]    = "Assume total responsabilidade pelo conteudo executado",
        [STR_DISCLAIMER_ITEM3_L2]    = "neste dispositivo.",
        [STR_DISCLAIMER_ACCEPT_TEXT] = " Concordo e Desejo Prosseguir",

        /* Join Hint In-Game */
        [STR_JOIN_HINT_PREFIX]       = "Aperte ",
        [STR_JOIN_HINT_SUFFIX]       = " para entrar com novo jogador",

        /* Abas do Launcher e OSD */
        [STR_TAB_GAMES]              = "[ JOGOS ]",
        [STR_TAB_VIDEO_AUDIO]        = "[ VIDEO & AUDIO ]",
        [STR_TAB_CONTROLS]           = "[ CONTROLE E ATALHOS ]",
        [STR_OSD_TAB_LINK]           = "[ LINK CABLE ]",
        [STR_OSD_TAB_CONTROLS]       = "[ CONTROLES ]",
        [STR_OSD_TITLE_FMT]          = "MENU v00.10 - CONTROLE P%d (%s)",
        [STR_OSD_LINK_TAG_FMT]       = "[LINK: %s]",

        /* Aba de Controles (DualShock 4 & GBA Classico) */
        [STR_CTRL_BTN_L]             = "Botao L",
        [STR_CTRL_SEL_KEY]           = "[TOUCHPAD / TRIANGULO]",
        [STR_CTRL_BTN_SELECT]        = "Botao SELECT GBA",
        [STR_CTRL_DPAD_KEY]          = "[D-PAD / ANALOG ESQ]",
        [STR_CTRL_MOVEMENT]          = "Movimento GBA",
        [STR_CTRL_AUDIO_FOCUS]       = "Alternar Foco de Audio",
        [STR_CTRL_BTN_R]             = "Botao R",
        [STR_CTRL_OPEN_MENU]         = "Abrir Menu / Entrar no Jogo",
        [STR_CTRL_SQUARE_CIRCLE]     = "[QUADRADO / BOLA]",
        [STR_CTRL_BTN_B]             = "Botao B",
        [STR_CTRL_CROSS]             = "[CRUZ (X)]",
        [STR_CTRL_BTN_A]             = "Botao A",
        [STR_CTRL_BTN_START]         = "Botao START GBA",
        [STR_CTRL_GBA_L]             = "Botao L GBA",
        [STR_CTRL_GBA_DPAD_DESC]     = "D-Pad / Analogico Esquerdo",
        [STR_CTRL_GBA_SEL_DESC]      = "Touchpad / Triangulo",
        [STR_CTRL_GBA_R]             = "Botao R GBA",
        [STR_CTRL_GBA_A]             = "Botao A GBA",
        [STR_CTRL_GBA_A_DESC]        = "Cruz (X)",
        [STR_CTRL_GBA_B]             = "Botao B GBA",
        [STR_CTRL_GBA_B_DESC]        = "Quadrado / Bola",
        [STR_CTRL_MP_TIP_1]          = "DICA MULTIPLAYER: Conecte ate 4 Controles DualShock 4 ao PS4.",
        [STR_CTRL_MP_TIP_2]          = "Nos Controles 2, 3 e 4, aperte OPTIONS para entrar na partida a qualquer momento via Drop-In.",

        /* Launcher - Lista de Jogos & USB */
        [STR_GAMES_LIST_TITLE]       = "LISTA DE JOGOS DISPONIVEIS",
        [STR_GAMES_PAGE_FMT]         = "Pagina %d de %d  (Total: %d ROMs)",
        [STR_GAMES_NAV_HINT]         = "[ESQUERDA / DIREITA] Pular Pagina   [TRIANGULO] Atualizar Lista",
        [STR_USB_INIT_STATUS]        = "USB: Inicializando portas /mnt/usb0..7...",
        [STR_USB_DETECTED_FMT]       = "USB: %s (%d ROMs USB detectadas)",
        [STR_USB_EMPTY_FMT]          = "USB: %s montado (0 ROMs encontradas na raiz/pastas)",
        [STR_USB_NONE]               = "USB: Nenhum pendrive detectado (Formate em FAT32 ou exFAT)",
        [STR_DEMO_ROM_NAME]          = "test.gba (Demo 4P Link Integrada)",
        [STR_SCAN_DONE_FMT]          = "Varredura concluida: %d ROM(s) encontradas (%d USB, %d Internas)",
        [STR_ROM_LOAD_ERR]           = "Erro ao carregar ROM. Aperte [TRIANGULO] para atualizar lista.",
        [STR_SIDE_GAME_DETAILS]      = "DETALHES DO JOGO",
        [STR_SIDE_FILE_NAME]         = "NOME DO ARQUIVO:",
        [STR_SIDE_FULL_PATH]         = "CAMINHO COMPLETO:",
        [STR_SIDE_STORAGE_DEV]       = "DISPOSITIVO DE ARMAZENAMENTO:",
        [STR_SIDE_STOR_USB]          = "Pendrive ou HD Externo USB (/mnt/usbX)",
        [STR_SIDE_STOR_INT]          = "Disco Rigido Interno do PS4 (/data)",
        [STR_SIDE_MP_SUPPORT]        = "SUPORTE MULTIPLAYER:",
        [STR_SIDE_MP_DESC]           = "Ate 4 Jogadores Simultaneos.\nLink Cable Virtual com Ring Buffer de alta velocidade em memoria RAM.",

        /* Configuracoes de Video, Audio & Idioma */
        [STR_SETTINGS_TITLE]         = "CONFIGURACOES DE VIDEO & AUDIO",
        [STR_SETTINGS_SUBTITLE]      = "Ajuste imagem, som e idioma para televisores HD e 4K",
        [STR_SETTING_FILTER]         = "Modo de Qualidade / Upscaling: < %s >",
        [STR_SETTING_SCALE]          = "Fator de Resolucao Interna: < %s >",
        [STR_SETTING_SCALE_OSD]      = "Fator Interno: < %s >%s",
        [STR_SETTING_SMOOTH]         = "Suavizacao (Bilinear): < %s >",
        [STR_SETTING_SHOW_FPS]       = "Contador de FPS: < %s >",
        [STR_SETTING_FPS_LIMIT]      = "Bloqueio de Framerate: < %s >",
        [STR_SETTING_FPS_LIMIT_OSD]  = "Bloqueio de FPS: < %s >",
        [STR_SETTING_AUDIO_MODE]     = "Audio: < %s >",
        [STR_SETTING_AUDIO_PLAYERS]  = "Audio dos Jogadores: < %s >",
        [STR_AUDIO_MIX_ALL]          = "Mix Todas as Telas",
        [STR_AUDIO_FOCUS_P1]         = "Foco P1",
        [STR_AUDIO_FOCUS_PN_FMT]     = "Foco Player P%d",
        [STR_ON]                     = "LIGADO",
        [STR_OFF]                    = "DESLIGADO",
        [STR_LAUNCHER_VA_HINT]       = "[ESQUERDA / DIREITA / X] Alterar   [D-PAD CIMA/BAIXO] Navegar",
        [STR_OSD_VA_HINT]            = "[ESQUERDA / DIREITA / X] Alterar em Tempo Real",
        [STR_FPS_LIMIT_TOAST_FMT]    = "Bloqueio de FPS: %s",

        /* Nomes de Modos de Video (video_split.c) */
        [STR_FILTER_NEAREST]         = "Pixel Puro (Nearest Neighbor)",
        [STR_FILTER_SCALENX]         = "ScaleNx (Scale2x / Scale3x / Scale4x)",
        [STR_FILTER_HQX]             = "HQx (aprox. hq2x / hq3x / hq4x)",
        [STR_FILTER_XBR]             = "xBR-lite (aprox. xBRZ / Super-xBR)",
        [STR_SCALE_1X]               = "1X (Nativo 240x160)",
        [STR_SCALE_2X]               = "2X (480x320)",
        [STR_SCALE_3X]               = "3X (720x480)",
        [STR_SCALE_4X]               = "4X (960x640)",
        [STR_SCALE_MAX]              = "6X Max (1440x960 Super-Sampling)",
        [STR_SMOOTH_ON]              = "LIGADO (Bilinear Suave)",
        [STR_SMOOTH_OFF]             = "DESLIGADO (Sharp / Raw Pixels)",
        [STR_DISPLAY_STRETCH]        = "Esticar Tela Inteira (Full 16:9)",
        [STR_DISPLAY_ASPECT]         = "Aspecto 3:2 Perfeito (Recomendado)",

        /* Painel Lateral de Configuracoes (Launcher & OSD) */
        [STR_SIDE_OPT_INFO]          = "INFORMACAO DA OPCAO",
        [STR_SIDE_VIDEO_TELEMETRY]   = "TELEMETRIA DE VIDEO",
        [STR_SIDE_VIDEO_OUT_LBL]     = "SAIDA DE VIDEO:",
        [STR_SIDE_GFX_ACCEL_LBL]     = "ACELERACAO GRAFICA:",
        [STR_SIDE_AUDIO_SHORTCUT_LBL]= "ATALHO DE AUDIO:",
        [STR_SIDE_AUDIO_SHORTCUT_VAL]= "[L3 + R3] Foco no seu P#",
        [STR_SIDE_OPT_INFO_LBL]      = "INFORMACOES DA OPCAO:",
        [STR_INFO_DISPLAY_TITLE]     = "PROPORCAO DA TELA:",
        [STR_INFO_DISPLAY_DESC_L]    = "3:2 Nativo: Preserva a geometria exata do Game Boy Advance com pillarboxing lateral.\n\n16:9 Esticado: Preenche toda a area da tela da TV sem bordas pretas.",
        [STR_INFO_DISPLAY_DESC_O]    = "Alterna entre proporcao 3:2 nativa e preenchimento 16:9 completo.",
        [STR_INFO_FILTER_TITLE]      = "ALGORITMO DE QUALIDADE:",
        [STR_INFO_FILTER_DESC_L]     = "Pixel Puro: Nearest-neighbor classico com pixels nitidos.\n\nScaleNx: Realce inteligente de bordas sem desfoque.\n\nHQx / xBR: Interpolacao vetorial avancada para visual moderno.",
        [STR_INFO_FILTER_DESC_O]     = "Filtro grafico de reconstrucao: Pixel Puro, ScaleNx ou HQx/xBR.",
        [STR_INFO_SCALE_TITLE]       = "FATOR INTERNO:",
        [STR_INFO_SCALE_DESC_L]      = "1X ate Max 4K: Define a resolucao interna de renderizacao antes da projecao na tela.\n\nEm 4 Jogadores, o emulador ajusta automaticamente para garantir 60 FPS cravados.",
        [STR_INFO_SCALE_DESC_O]      = "Multiplicador de escala interna para renderizacao de alta definicao.",
        [STR_INFO_SMOOTH_TITLE]      = "SUAVIZACAO BILINEAR:",
        [STR_INFO_SMOOTH_DESC_L]     = "Aplica interpolacao bilinear na apresentacao final, eliminando cintilacoes e suavizando gradientes.",
        [STR_INFO_SMOOTH_DESC_O]     = "Filtro bilinear para remocao de flicker em telas planas modernas.",
        [STR_INFO_FPS_TITLE]         = "CONTADOR DE FPS:",
        [STR_INFO_FPS_DESC_L]        = "Exibe a taxa real de quadros por segundo no canto superior esquerdo da TV durante o jogo.",
        [STR_INFO_FPS_DESC_O]        = "Liga ou desliga o medidor de FPS no canto da tela.",
        [STR_INFO_FPS_LIM_TITLE]     = "BLOQUEIO DE FPS:",
        [STR_INFO_FPS_LIM_DESC_L]    = "60 FPS: Velocidade nominal original do Game Boy Advance.\n\n30 FPS: Modo alternativo de sincronizacao para economia de processamento.",
        [STR_INFO_FPS_LIM_DESC_O]    = "Trava a taxa de quadros em 60 FPS ou 30 FPS.",
        [STR_INFO_AUDIO_TITLE]       = "MODO DE AUDIO:",
        [STR_INFO_AUDIO_DESC_L]      = "Mix: Mistura o som de todos os jogadores ativos (25% cada).\n\nFoco: Concentra 100% do volume na tela do Jogador 1 (atalho L3+R3).",
        [STR_INFO_AUDIO_DESC_O]      = "Alterna audio entre mix geral e foco isolado em uma das telas.",
        [STR_INFO_SFX_TITLE]         = "SONS DA INTERFACE:",
        [STR_INFO_SFX_DESC]          = "Ativa ou desativa os efeitos sonoros ao navegar pelas abas e opcoes dos menus. O audio do jogo em execucao continua funcionando normalmente.",
        [STR_INFO_LANG_TITLE]        = "IDIOMA DA INTERFACE:",
        [STR_INFO_LANG_DESC]         = "Alterna instantaneamente todos os textos do emulador entre Portugues e Ingles, salvando sua preferencia em /data/PSgba/config.bin.",
        [STR_INFO_EXIT_TITLE]        = "SAIR DO EMULADOR:",
        [STR_INFO_EXIT_DESC]         = "Encerra a aplicacao de forma segura, sincronizando saves em disco, liberando controles Bluetooth e restaurando o estado original do kernel.",

        /* OSD - Aba de Saves */
        [STR_SAVES_HEADER]           = "GERENCIAMENTO DE JOGO & SAVES",
        [STR_SAVE_TYPE_GRP]          = "Tipo de Save: < INDIVIDUAL / [GRUPO] >",
        [STR_SAVE_TYPE_IND]          = "Tipo de Save: < [INDIVIDUAL] / GRUPO >",
        [STR_SAVE_ACTION_SAVE]       = "Salvar Estado",
        [STR_SAVE_ACTION_LOAD]       = "Carregar Estado",
        [STR_SAVE_PREVIEW_HDR_FMT]   = "PREVIEW DO SAVE - SLOT %d",
        [STR_SAVE_NO_THUMB]          = "[ SAVE GRAVADO - SEM MINIATURA ]",
        [STR_SAVE_EMPTY_SLOT]        = "[ SLOT VAZIO ]",
        [STR_SAVE_DATE_FMT]          = "Data: %s",
        [STR_SAVE_TIME_FMT]          = "Hora: %s",
        [STR_SAVE_DATE_EMPTY]        = "Data: --/--/----",
        [STR_SAVE_TIME_EMPTY]        = "Hora: --:--:--",
        [STR_TOAST_SAVE_MODE_GRP]    = "[MODO] Tipo de Save: GRUPO",
        [STR_TOAST_SAVE_MODE_IND]    = "[MODO] Tipo de Save: INDIVIDUAL",
        [STR_TOAST_SLOT_SEL_FMT]     = "[SLOT] Slot %d selecionado",
        [STR_TOAST_GRP_SAVED_FMT]    = "[SUCESSO] Estado de Grupo salvo no Slot %d!",
        [STR_TOAST_GRP_SAVE_ERR_FMT] = "[ERRO] Falha ao salvar Estado de Grupo no Slot %d!",
        [STR_TOAST_IND_SAVED_FMT]    = "[SUCESSO] Estado salvo no Slot %d!",
        [STR_TOAST_IND_SAVE_ERR_FMT] = "[ERRO] Falha ao salvar Estado no Slot %d!",
        [STR_TOAST_GRP_LOADED_FMT]   = "[SUCESSO] Estado de Grupo carregado do Slot %d!",
        [STR_TOAST_IND_LOADED_FMT]   = "[SUCESSO] Estado carregado do Slot %d!",
        [STR_TOAST_SLOT_EMPTY_FMT]   = "[ERRO] Slot %d vazio!",

        /* OSD - Aba de Cheats */
        [STR_CHEATS_HEADER]          = "TRAPACAS & CHEATS (ACTION REPLAY / GAMESHARK)",
        [STR_CHEATS_RELOADED_FMT]    = "P%d: %d trapaca(s) recarregada(s)!",
        [STR_CHEATS_TOGGLED_FMT]     = "P%d: %s [%s] (%d opcodes%s)",
        [STR_CHEATS_ENABLED]         = "ATIVADO",
        [STR_CHEATS_DISABLED]        = "DESATIVADO",
        [STR_CHEATS_ZERO_RECOGNIZED] = "< Arquivo encontrado, mas 0 codigos reconhecidos >",
        [STR_CHEATS_FILE_FMT]        = "Arquivo: %s",
        [STR_CHEATS_BAD_FORMAT]      = "O formato do arquivo pode nao conter codigos compativeis com GBA.",
        [STR_CHEATS_PRESS_TRIANGLE]  = "Aperte [TRIANGULO] para tentar recarregar novamente.",
        [STR_CHEATS_NONE_FOUND]      = "< Nenhum arquivo de cheat (.cht / .cheats) encontrado >",
        [STR_CHEATS_DIAG_FMT]        = "Diagnostico: %s",
        [STR_CHEATS_USB_HINT_HDR]    = "Coloque o arquivo no Pendrive USB com qualquer uma das opcoes:",
        [STR_CHEATS_USB_HINT_1]      = "1. Mesmo nome da ROM (<jogo>.cht ou <jogo>.cheats)",
        [STR_CHEATS_USB_HINT_2]      = "2. Codigo do jogo (ex: BPRE.cht) na raiz ou pasta cheat/ do USB",
        [STR_CHEATS_USB_HINT_3]      = "Aperte [TRIANGULO] a qualquer momento para buscar novamente no USB.",
        [STR_CHEATS_ORIGIN_FMT]      = "Origem: %s  |  Jogador P%d (%d codigos)",
        [STR_CHEATS_UNNAMED]         = "Trapaca Sem Nome",
        [STR_CHEATS_PAGE_FMT]        = "Codigo %d de %d  [L2 / R2: Pagina]",
        [STR_SIDE_CHEAT_DETAILS]     = "DETALHES DO CHEAT",
        [STR_SIDE_ACTIVE_PLAYER]     = "JOGADOR ATIVO:",
        [STR_SIDE_PLAYER_INST_FMT]   = "Controle P%d (Instancia %d)",
        [STR_SIDE_CHEAT_FILE]        = "ARQUIVO DE TRAPACA:",
        [STR_SIDE_NO_FILE]           = "<Nenhum arquivo>",
        [STR_SIDE_SEARCH_STATUS]     = "STATUS DA BUSCA / LEITURA:",
        [STR_SIDE_SEL_CODE]          = "CODIGO SELECIONADO:",
        [STR_SIDE_CHEAT_STATUS]      = "STATUS DA TRAPACA:",
        [STR_SIDE_CHEAT_ON]          = "[X] ATIVADO",
        [STR_SIDE_CHEAT_OFF]         = "[ ] DESATIVADO",
        [STR_SIDE_COMPILED_CODES]    = "CODIGOS COMPILADOS:",
        [STR_SIDE_OPCODES_FMT]       = "%d opcode(s) em memoria%s",
        [STR_SIDE_MASTER_NOTE]       = "* Codigo Mestre (M): auto-ativado quando qualquer cheat e ligado.",
        [STR_SIDE_HOW_IT_WORKS]      = "COMO FUNCIONA:",
        [STR_SIDE_CHEATS_HOW_DESC]   = "Os cheats sao interpretados pela libmgba nativa (GameShark, Action Replay, CodeBreaker). A ativacao modifica a memoria do jogo em tempo real sem travar.",

        /* OSD - Aba Link Cable */
        [STR_LINK_HEADER]            = "CONEXAO E ESTADO DO LINK CABLE",
        [STR_LINK_CONNECTED]         = "CONECTADO",
        [STR_LINK_DISCONNECTED]      = "DESCONECTADO",
        [STR_LINK_RECONNECTING]      = "RECONECTANDO...",
        [STR_LINK_STATE_FMT]         = "Estado do Cabo: < %s >",
        [STR_LINK_HOTPLUG_ITEM]      = "[X] Reconectar Link Cable (Simular Hotplug)",
        [STR_LINK_MODE_ITEM]         = "Modo do Cabo: < Multi-player (GBA Link) >",
        [STR_LINK_DEV_TELEMETRY_FMT] = "[%c] Opcoes de Desenvolvedor / Telemetria",
        [STR_LINK_TOAST_CONN]        = "Link Cable: CONECTADO",
        [STR_LINK_TOAST_DISC]        = "Link Cable: DESCONECTADO",
        [STR_LINK_TOAST_HOTPLUG]     = "Reconectando Link Cable... Volte ao jogo!",
        [STR_LINK_TELE_HEADER]       = "TELEMETRIA DETALHADA DAS 4 PORTAS GBA SIO",
        [STR_LINK_PKTS_FMT]          = "Pacotes trocados: %u  |  Buffer: Lockless Ring Buffer em RAM",
        [STR_LINK_MODE_MULTI]        = "MULTI (Multiplayer 4P)",
        [STR_LINK_MODE_NORM32]       = "NORM32 (Transferencia 32-bit)",
        [STR_LINK_MODE_NORM8]        = "NORM8 (Transferencia 8-bit)",
        [STR_LINK_MODE_GPIO]         = "GPIO (Linha)",
        [STR_LINK_PLAYER_MODE_FMT]   = "GBA Player %d (%s): Modo [%s]",
        [STR_LINK_HOW_TITLE]         = "COMO FUNCIONA O LINK CABLE:",
        [STR_LINK_HOW_L1]            = "- O cabo virtual conecta em memoria todas as portas GBA em tempo real.",
        [STR_LINK_HOW_L2]            = "- Se perder sincronia, selecione reconectar.",

        /* Rodapes (Footers) */
        [STR_FOOTER_LAUNCHER_CTRL]   = "[D-PAD] Navegar   [O] Voltar   [TOUCHPAD] Alternar Visualizacao (DualShock 4 / Game Boy Advance)",
        [STR_FOOTER_LAUNCHER_DEF]    = "[D-PAD] Navegar   [X] Confirmar   [O] Voltar   [TOUCHPAD] Ajuda/Detalhes   [TRIANGULO] Atualizar ROMs",
        [STR_FOOTER_OSD_CTRL]        = "[D-PAD] Navegar   [O / OPTIONS] Voltar ao Jogo   [TOUCHPAD] Alternar Visualizacao (DualShock 4 / Game Boy Advance)",
        [STR_FOOTER_OSD_CHEATS]      = "[D-PAD] Navegar   [X] Ativar/Desativar Trapaca   [L2 / R2] Pagina   [TRIANGULO] Buscar .cht Novamente   [O / OPTIONS] Voltar",
        [STR_FOOTER_OSD_DEF]         = "[D-PAD] Navegar / Trocar Slot   [X] Confirmar   [O / OPTIONS] Voltar ao Jogo   [TOUCHPAD] Ajuda/Detalhes"
    },

    /* ======================================================================
     * LANG_EN_US - English (United States / International)
     * ====================================================================== */
    [LANG_EN_US] = {
        [STR_APP_TITLE]              = "PSgba - GBA 4-WAY SPLIT-SCREEN EMULATOR v00.10",
        [STR_PRESS_ANY_BUTTON]       = "Press any button to start",
        [STR_MENU_RESUME]            = "Resume Game",
        [STR_MENU_RESET]             = "Return to Launcher (Close ROM)",
        [STR_MENU_SAVES]             = "[ GAME & SAVES ]",
        [STR_MENU_CHEATS]            = "[ CHEATS ]",
        [STR_MENU_SETTINGS]          = "[ VIDEO & AUDIO ]",
        [STR_MENU_EXIT]              = "Exit Emulator (Close Application)",
        [STR_SETTING_LANGUAGE]       = "Idioma / Language: < Portugues / [English] >",
        [STR_SETTING_AUDIO_SFX]      = "Menu SFX: < %s >",
        [STR_SETTING_VIDEO_MODE]     = "Aspect Ratio: < %s >",
        [STR_DISCLAIMER_TITLE]       = "[ LEGAL NOTICE & COMMITMENT ]",
        [STR_DISCLAIMER_BODY]        = "This project was developed strictly for historical preservation and software engineering purposes.",
        [STR_DISCLAIMER_ACCEPT]      = "[X] I Agree and Wish to Proceed",

        /* Splash Screen & Disclaimer */
        [STR_SPLASH_CONNECTING]      = "CONNECTING 4 PLAYERS...",
        [STR_SPLASH_SUBTITLE]        = "4-WAY SPLIT-SCREEN EMULATOR v00.10",
        [STR_SPLASH_DEVELOPED_BY]    = "DEVELOPED BY",
        [STR_SPLASH_SKIP]            = "[X / OPTIONS] Skip",
        [STR_DISCLAIMER_P1_L1]       = "This project was developed strictly for historical",
        [STR_DISCLAIMER_P1_L2]       = "preservation and software engineering purposes.",
        [STR_DISCLAIMER_ALERT]       = "We are COMPLETELY AGAINST PIRACY in any form.",
        [STR_DISCLAIMER_INTRO]       = "By using PSgba, you declare and agree that:",
        [STR_DISCLAIMER_ITEM1_L1]    = "You will not use piracy to obtain any ROMs",
        [STR_DISCLAIMER_ITEM1_L2]    = "or BIOS files.",
        [STR_DISCLAIMER_ITEM2_L1]    = "You will exclusively use backup copies dumped from",
        [STR_DISCLAIMER_ITEM2_L2]    = "your own legally acquired original game cartridges.",
        [STR_DISCLAIMER_ITEM3_L1]    = "You assume full responsibility for the content executed",
        [STR_DISCLAIMER_ITEM3_L2]    = "on this device.",
        [STR_DISCLAIMER_ACCEPT_TEXT] = " I Agree and Wish to Proceed",

        /* Join Hint In-Game */
        [STR_JOIN_HINT_PREFIX]       = "Press ",
        [STR_JOIN_HINT_SUFFIX]       = " to join as a new player",

        /* Abas do Launcher e OSD */
        [STR_TAB_GAMES]              = "[ GAMES ]",
        [STR_TAB_VIDEO_AUDIO]        = "[ VIDEO & AUDIO ]",
        [STR_TAB_CONTROLS]           = "[ CONTROLS & SHORTCUTS ]",
        [STR_OSD_TAB_LINK]           = "[ LINK CABLE ]",
        [STR_OSD_TAB_CONTROLS]       = "[ CONTROLS ]",
        [STR_OSD_TITLE_FMT]          = "MENU v00.10 - CONTROLLER P%d (%s)",
        [STR_OSD_LINK_TAG_FMT]       = "[LINK: %s]",

        /* Aba de Controles (DualShock 4 & GBA Classico) */
        [STR_CTRL_BTN_L]             = "Button L",
        [STR_CTRL_SEL_KEY]           = "[TOUCHPAD / TRIANGLE]",
        [STR_CTRL_BTN_SELECT]        = "GBA SELECT Button",
        [STR_CTRL_DPAD_KEY]          = "[D-PAD / LEFT STICK]",
        [STR_CTRL_MOVEMENT]          = "GBA Movement",
        [STR_CTRL_AUDIO_FOCUS]       = "Toggle Audio Focus",
        [STR_CTRL_BTN_R]             = "Button R",
        [STR_CTRL_OPEN_MENU]         = "Open Menu / Join Game",
        [STR_CTRL_SQUARE_CIRCLE]     = "[SQUARE / CIRCLE]",
        [STR_CTRL_BTN_B]             = "Button B",
        [STR_CTRL_CROSS]             = "[CROSS (X)]",
        [STR_CTRL_BTN_A]             = "Button A",
        [STR_CTRL_BTN_START]         = "GBA START Button",
        [STR_CTRL_GBA_L]             = "GBA L Button",
        [STR_CTRL_GBA_DPAD_DESC]     = "D-Pad / Left Analog Stick",
        [STR_CTRL_GBA_SEL_DESC]      = "Touchpad / Triangle",
        [STR_CTRL_GBA_R]             = "GBA R Button",
        [STR_CTRL_GBA_A]             = "GBA A Button",
        [STR_CTRL_GBA_A_DESC]        = "Cross (X)",
        [STR_CTRL_GBA_B]             = "GBA B Button",
        [STR_CTRL_GBA_B_DESC]        = "Square / Circle",
        [STR_CTRL_MP_TIP_1]          = "MULTIPLAYER TIP: Connect up to 4 DualShock 4 Controllers to your PS4.",
        [STR_CTRL_MP_TIP_2]          = "On Controllers 2, 3, and 4, press OPTIONS to join the match at any time via Drop-In.",

        /* Launcher - Lista de Jogos & USB */
        [STR_GAMES_LIST_TITLE]       = "AVAILABLE GAMES LIST",
        [STR_GAMES_PAGE_FMT]         = "Page %d of %d  (Total: %d ROMs)",
        [STR_GAMES_NAV_HINT]         = "[LEFT / RIGHT] Page Jump   [TRIANGLE] Refresh List",
        [STR_USB_INIT_STATUS]        = "USB: Initializing ports /mnt/usb0..7...",
        [STR_USB_DETECTED_FMT]       = "USB: %s (%d USB ROMs detected)",
        [STR_USB_EMPTY_FMT]          = "USB: %s mounted (0 ROMs found in root/folders)",
        [STR_USB_NONE]               = "USB: No USB drive detected (Format as FAT32 or exFAT)",
        [STR_DEMO_ROM_NAME]          = "test.gba (Built-in 4P Link Demo)",
        [STR_SCAN_DONE_FMT]          = "Scan complete: %d ROM(s) found (%d USB, %d Internal)",
        [STR_ROM_LOAD_ERR]           = "Error loading ROM. Press [TRIANGLE] to refresh list.",
        [STR_SIDE_GAME_DETAILS]      = "GAME DETAILS",
        [STR_SIDE_FILE_NAME]         = "FILE NAME:",
        [STR_SIDE_FULL_PATH]         = "FULL PATH:",
        [STR_SIDE_STORAGE_DEV]       = "STORAGE DEVICE:",
        [STR_SIDE_STOR_USB]          = "USB Flash Drive or External Drive (/mnt/usbX)",
        [STR_SIDE_STOR_INT]          = "PS4 Internal Hard Drive (/data)",
        [STR_SIDE_MP_SUPPORT]        = "MULTIPLAYER SUPPORT:",
        [STR_SIDE_MP_DESC]           = "Up to 4 Simultaneous Players.\nVirtual Link Cable with high-speed in-RAM Ring Buffer.",

        /* Configuracoes de Video, Audio & Idioma */
        [STR_SETTINGS_TITLE]         = "VIDEO & AUDIO SETTINGS",
        [STR_SETTINGS_SUBTITLE]      = "Adjust display, sound, and language for HD and 4K TVs",
        [STR_SETTING_FILTER]         = "Upscaling / Quality Mode: < %s >",
        [STR_SETTING_SCALE]          = "Internal Resolution Scale: < %s >",
        [STR_SETTING_SCALE_OSD]      = "Internal Scale: < %s >%s",
        [STR_SETTING_SMOOTH]         = "Smoothing (Bilinear): < %s >",
        [STR_SETTING_SHOW_FPS]       = "FPS Counter: < %s >",
        [STR_SETTING_FPS_LIMIT]      = "Framerate Limit: < %s >",
        [STR_SETTING_FPS_LIMIT_OSD]  = "FPS Limit: < %s >",
        [STR_SETTING_AUDIO_MODE]     = "Audio: < %s >",
        [STR_SETTING_AUDIO_PLAYERS]  = "Players Audio: < %s >",
        [STR_AUDIO_MIX_ALL]          = "Mix All Screens",
        [STR_AUDIO_FOCUS_P1]         = "Focus P1",
        [STR_AUDIO_FOCUS_PN_FMT]     = "Focus Player P%d",
        [STR_ON]                     = "ON",
        [STR_OFF]                    = "OFF",
        [STR_LAUNCHER_VA_HINT]       = "[LEFT / RIGHT / X] Change   [D-PAD UP/DOWN] Navigate",
        [STR_OSD_VA_HINT]            = "[LEFT / RIGHT / X] Change in Real Time",
        [STR_FPS_LIMIT_TOAST_FMT]    = "FPS Limit: %s",

        /* Nomes de Modos de Video (video_split.c) */
        [STR_FILTER_NEAREST]         = "Pure Pixel (Nearest Neighbor)",
        [STR_FILTER_SCALENX]         = "ScaleNx (Scale2x / Scale3x / Scale4x)",
        [STR_FILTER_HQX]             = "HQx (approx. hq2x / hq3x / hq4x)",
        [STR_FILTER_XBR]             = "xBR-lite (approx. xBRZ / Super-xBR)",
        [STR_SCALE_1X]               = "1X (Native 240x160)",
        [STR_SCALE_2X]               = "2X (480x320)",
        [STR_SCALE_3X]               = "3X (720x480)",
        [STR_SCALE_4X]               = "4X (960x640)",
        [STR_SCALE_MAX]              = "6X Max (1440x960 Super-Sampling)",
        [STR_SMOOTH_ON]              = "ON (Smooth Bilinear)",
        [STR_SMOOTH_OFF]             = "OFF (Sharp / Raw Pixels)",
        [STR_DISPLAY_STRETCH]        = "Stretch Full Screen (Full 16:9)",
        [STR_DISPLAY_ASPECT]         = "Perfect 3:2 Aspect (Recommended)",

        /* Painel Lateral de Configuracoes (Launcher & OSD) */
        [STR_SIDE_OPT_INFO]          = "OPTION INFORMATION",
        [STR_SIDE_VIDEO_TELEMETRY]   = "VIDEO TELEMETRY",
        [STR_SIDE_VIDEO_OUT_LBL]     = "VIDEO OUTPUT:",
        [STR_SIDE_GFX_ACCEL_LBL]     = "GRAPHICS ACCELERATION:",
        [STR_SIDE_AUDIO_SHORTCUT_LBL]= "AUDIO SHORTCUT:",
        [STR_SIDE_AUDIO_SHORTCUT_VAL]= "[L3 + R3] Focus on your P#",
        [STR_SIDE_OPT_INFO_LBL]      = "OPTION INFORMATION:",
        [STR_INFO_DISPLAY_TITLE]     = "ASPECT RATIO:",
        [STR_INFO_DISPLAY_DESC_L]    = "Native 3:2: Preserves exact Game Boy Advance geometry with side pillarboxing.\n\n16:9 Stretched: Fills the entire TV screen area with no black bars.",
        [STR_INFO_DISPLAY_DESC_O]    = "Switches between native 3:2 aspect ratio and full 16:9 screen fill.",
        [STR_INFO_FILTER_TITLE]      = "QUALITY ALGORITHM:",
        [STR_INFO_FILTER_DESC_L]     = "Pure Pixel: Classic nearest-neighbor with sharp pixels.\n\nScaleNx: Smart edge enhancement without blur.\n\nHQx / xBR: Advanced vector interpolation for a modern look.",
        [STR_INFO_FILTER_DESC_O]     = "Reconstruction graphics filter: Pure Pixel, ScaleNx, or HQx/xBR.",
        [STR_INFO_SCALE_TITLE]       = "INTERNAL SCALE:",
        [STR_INFO_SCALE_DESC_L]      = "1X to Max 4K: Sets internal rendering resolution before screen projection.\n\nIn 4-Player mode, the emulator adjusts automatically to guarantee locked 60 FPS.",
        [STR_INFO_SCALE_DESC_O]      = "Internal resolution multiplier for high-definition rendering.",
        [STR_INFO_SMOOTH_TITLE]      = "BILINEAR SMOOTHING:",
        [STR_INFO_SMOOTH_DESC_L]     = "Applies bilinear interpolation to the final presentation, eliminating shimmer and smoothing gradients.",
        [STR_INFO_SMOOTH_DESC_O]     = "Bilinear filter to remove pixel shimmer on modern flat displays.",
        [STR_INFO_FPS_TITLE]         = "FPS COUNTER:",
        [STR_INFO_FPS_DESC_L]        = "Displays the real-time frames per second rate in the top-left corner of the TV during gameplay.",
        [STR_INFO_FPS_DESC_O]        = "Toggles the on-screen FPS counter in the top-left corner.",
        [STR_INFO_FPS_LIM_TITLE]     = "FPS LIMIT:",
        [STR_INFO_FPS_LIM_DESC_L]    = "60 FPS: Original nominal speed of the Game Boy Advance.\n\n30 FPS: Alternative frame pacing mode for reduced rendering load.",
        [STR_INFO_FPS_LIM_DESC_O]    = "Locks video presentation rate to 60 FPS or 30 FPS.",
        [STR_INFO_AUDIO_TITLE]       = "AUDIO MODE:",
        [STR_INFO_AUDIO_DESC_L]      = "Mix: Blends audio from all active players (25% each).\n\nFocus: Concentrates 100% volume on Player 1's screen (L3+R3 shortcut).",
        [STR_INFO_AUDIO_DESC_O]      = "Switches audio between all-player mix and isolated single-player focus.",
        [STR_INFO_SFX_TITLE]         = "INTERFACE SOUND EFFECTS:",
        [STR_INFO_SFX_DESC]          = "Enables or disables sound effects when navigating menu tabs and options. Running game audio continues working normally.",
        [STR_INFO_LANG_TITLE]        = "INTERFACE LANGUAGE:",
        [STR_INFO_LANG_DESC]         = "Instantly switches all emulator UI texts between Portuguese and English, saving your preference to /data/PSgba/config.bin.",
        [STR_INFO_EXIT_TITLE]        = "EXIT EMULATOR:",
        [STR_INFO_EXIT_DESC]         = "Safely closes the application, syncing saves to disk, releasing Bluetooth controllers, and restoring original kernel state.",

        /* OSD - Aba de Saves */
        [STR_SAVES_HEADER]           = "GAME & SAVES MANAGEMENT",
        [STR_SAVE_TYPE_GRP]          = "Save Type: < INDIVIDUAL / [GROUP] >",
        [STR_SAVE_TYPE_IND]          = "Save Type: < [INDIVIDUAL] / GROUP >",
        [STR_SAVE_ACTION_SAVE]       = "Save State",
        [STR_SAVE_ACTION_LOAD]       = "Load State",
        [STR_SAVE_PREVIEW_HDR_FMT]   = "SAVE PREVIEW - SLOT %d",
        [STR_SAVE_NO_THUMB]          = "[ SAVE RECORDED - NO THUMBNAIL ]",
        [STR_SAVE_EMPTY_SLOT]        = "[ EMPTY SLOT ]",
        [STR_SAVE_DATE_FMT]          = "Date: %s",
        [STR_SAVE_TIME_FMT]          = "Time: %s",
        [STR_SAVE_DATE_EMPTY]        = "Date: --/--/----",
        [STR_SAVE_TIME_EMPTY]        = "Time: --:--:--",
        [STR_TOAST_SAVE_MODE_GRP]    = "[MODE] Save Type: GROUP",
        [STR_TOAST_SAVE_MODE_IND]    = "[MODE] Save Type: INDIVIDUAL",
        [STR_TOAST_SLOT_SEL_FMT]     = "[SLOT] Slot %d selected",
        [STR_TOAST_GRP_SAVED_FMT]    = "[SUCCESS] Group State saved to Slot %d!",
        [STR_TOAST_GRP_SAVE_ERR_FMT] = "[ERROR] Failed to save Group State to Slot %d!",
        [STR_TOAST_IND_SAVED_FMT]    = "[SUCCESS] State saved to Slot %d!",
        [STR_TOAST_IND_SAVE_ERR_FMT] = "[ERROR] Failed to save State to Slot %d!",
        [STR_TOAST_GRP_LOADED_FMT]   = "[SUCCESS] Group State loaded from Slot %d!",
        [STR_TOAST_IND_LOADED_FMT]   = "[SUCCESS] State loaded from Slot %d!",
        [STR_TOAST_SLOT_EMPTY_FMT]   = "[ERROR] Slot %d is empty!",

        /* OSD - Aba de Cheats */
        [STR_CHEATS_HEADER]          = "CHEATS (ACTION REPLAY / GAMESHARK)",
        [STR_CHEATS_RELOADED_FMT]    = "P%d: %d cheat(s) reloaded!",
        [STR_CHEATS_TOGGLED_FMT]     = "P%d: %s [%s] (%d opcodes%s)",
        [STR_CHEATS_ENABLED]         = "ENABLED",
        [STR_CHEATS_DISABLED]        = "DISABLED",
        [STR_CHEATS_ZERO_RECOGNIZED] = "< File found, but 0 codes recognized >",
        [STR_CHEATS_FILE_FMT]        = "File: %s",
        [STR_CHEATS_BAD_FORMAT]      = "The file format may not contain GBA-compatible cheat codes.",
        [STR_CHEATS_PRESS_TRIANGLE]  = "Press [TRIANGLE] to try reloading again.",
        [STR_CHEATS_NONE_FOUND]      = "< No cheat file (.cht / .cheats) found >",
        [STR_CHEATS_DIAG_FMT]        = "Diagnostics: %s",
        [STR_CHEATS_USB_HINT_HDR]    = "Place the file on your USB drive using any of these options:",
        [STR_CHEATS_USB_HINT_1]      = "1. Same name as the ROM (<game>.cht or <game>.cheats)",
        [STR_CHEATS_USB_HINT_2]      = "2. Game ID code (e.g. BPRE.cht) in USB root or cheat/ folder",
        [STR_CHEATS_USB_HINT_3]      = "Press [TRIANGLE] at any time to rescan the USB drive.",
        [STR_CHEATS_ORIGIN_FMT]      = "Source: %s  |  Player P%d (%d codes)",
        [STR_CHEATS_UNNAMED]         = "Unnamed Cheat",
        [STR_CHEATS_PAGE_FMT]        = "Code %d of %d  [L2 / R2: Page]",
        [STR_SIDE_CHEAT_DETAILS]     = "CHEAT DETAILS",
        [STR_SIDE_ACTIVE_PLAYER]     = "ACTIVE PLAYER:",
        [STR_SIDE_PLAYER_INST_FMT]   = "Controller P%d (Instance %d)",
        [STR_SIDE_CHEAT_FILE]        = "CHEAT FILE:",
        [STR_SIDE_NO_FILE]           = "<No file>",
        [STR_SIDE_SEARCH_STATUS]     = "SEARCH / READ STATUS:",
        [STR_SIDE_SEL_CODE]          = "SELECTED CODE:",
        [STR_SIDE_CHEAT_STATUS]      = "CHEAT STATUS:",
        [STR_SIDE_CHEAT_ON]          = "[X] ENABLED",
        [STR_SIDE_CHEAT_OFF]         = "[ ] DISABLED",
        [STR_SIDE_COMPILED_CODES]    = "COMPILED CODES:",
        [STR_SIDE_OPCODES_FMT]       = "%d opcode(s) in memory%s",
        [STR_SIDE_MASTER_NOTE]       = "* Master Code (M): auto-enabled whenever any cheat is active.",
        [STR_SIDE_HOW_IT_WORKS]      = "HOW IT WORKS:",
        [STR_SIDE_CHEATS_HOW_DESC]   = "Cheats are interpreted by native libmgba (GameShark, Action Replay, CodeBreaker). Toggling modifies game memory in real time without freezing.",

        /* OSD - Aba Link Cable */
        [STR_LINK_HEADER]            = "LINK CABLE CONNECTION & STATUS",
        [STR_LINK_CONNECTED]         = "CONNECTED",
        [STR_LINK_DISCONNECTED]      = "DISCONNECTED",
        [STR_LINK_RECONNECTING]      = "RECONNECTING...",
        [STR_LINK_STATE_FMT]         = "Cable Status: < %s >",
        [STR_LINK_HOTPLUG_ITEM]      = "[X] Reconnect Link Cable (Simulate Hotplug)",
        [STR_LINK_MODE_ITEM]         = "Cable Mode: < Multi-player (GBA Link) >",
        [STR_LINK_DEV_TELEMETRY_FMT] = "[%c] Developer Options / Telemetry",
        [STR_LINK_TOAST_CONN]        = "Link Cable: CONNECTED",
        [STR_LINK_TOAST_DISC]        = "Link Cable: DISCONNECTED",
        [STR_LINK_TOAST_HOTPLUG]     = "Reconnecting Link Cable... Return to game!",
        [STR_LINK_TELE_HEADER]       = "DETAILED TELEMETRY OF THE 4 GBA SIO PORTS",
        [STR_LINK_PKTS_FMT]          = "Packets exchanged: %u  |  Buffer: Lockless RAM Ring Buffer",
        [STR_LINK_MODE_MULTI]        = "MULTI (4P Multiplayer)",
        [STR_LINK_MODE_NORM32]       = "NORM32 (32-bit Transfer)",
        [STR_LINK_MODE_NORM8]        = "NORM8 (8-bit Transfer)",
        [STR_LINK_MODE_GPIO]         = "GPIO (General Purpose)",
        [STR_LINK_PLAYER_MODE_FMT]   = "GBA Player %d (%s): Mode [%s]",
        [STR_LINK_HOW_TITLE]         = "HOW THE LINK CABLE WORKS:",
        [STR_LINK_HOW_L1]            = "- The virtual cable connects all GBA ports in memory in real time.",
        [STR_LINK_HOW_L2]            = "- If sync is lost, select reconnect.",

        /* Rodapes (Footers) */
        [STR_FOOTER_LAUNCHER_CTRL]   = "[D-PAD] Navigate   [O] Back   [TOUCHPAD] Toggle View (DualShock 4 / Game Boy Advance)",
        [STR_FOOTER_LAUNCHER_DEF]    = "[D-PAD] Navigate   [X] Confirm   [O] Back   [TOUCHPAD] Help/Details   [TRIANGLE] Refresh ROMs",
        [STR_FOOTER_OSD_CTRL]        = "[D-PAD] Navigate   [O / OPTIONS] Resume Game   [TOUCHPAD] Toggle View (DualShock 4 / Game Boy Advance)",
        [STR_FOOTER_OSD_CHEATS]      = "[D-PAD] Navigate   [X] Toggle Cheat   [L2 / R2] Page   [TRIANGLE] Rescan .cht   [O / OPTIONS] Resume",
        [STR_FOOTER_OSD_DEF]         = "[D-PAD] Navigate / Change Slot   [X] Confirm   [O / OPTIONS] Resume Game   [TOUCHPAD] Help/Details"
    }
};

bool i18n_load_config(void) {
    ps4_jailbreak_system();
    FILE* f = fopen(PSGBA_CONFIG_PATH, "rb");
    if (!f) {
        return false;
    }

    PS4GBA_ConfigData cfg;
    memset(&cfg, 0, sizeof(cfg));
    size_t n = fread(&cfg, 1, sizeof(cfg), f);
    fclose(f);

    if (n >= sizeof(uint32_t) * 3 &&
        cfg.magic == PSGBA_CONFIG_MAGIC &&
        cfg.version == PSGBA_CONFIG_VERSION &&
        cfg.language >= 0 && cfg.language < (int32_t)LANG_COUNT) {
        g_current_language = (AppLanguage)cfg.language;
        return true;
    }

    /* Suporte também a formato compacto de 4 bytes (int32_t puro) */
    if (n == sizeof(int32_t)) {
        int32_t raw_lang = 0;
        memcpy(&raw_lang, &cfg, sizeof(int32_t));
        if (raw_lang >= 0 && raw_lang < (int32_t)LANG_COUNT) {
            g_current_language = (AppLanguage)raw_lang;
            return true;
        }
    }

    return false;
}

bool i18n_save_config(void) {
    ps4_jailbreak_system();
    mkdir("/data", 0777);
    mkdir(PSGBA_CONFIG_DIR, 0777);

    FILE* f = fopen(PSGBA_CONFIG_PATH, "wb");
    if (!f) {
        return false;
    }

    PS4GBA_ConfigData cfg;
    cfg.magic    = PSGBA_CONFIG_MAGIC;
    cfg.version  = PSGBA_CONFIG_VERSION;
    cfg.language = (int32_t)g_current_language;
    cfg.reserved = 0;

    size_t written = fwrite(&cfg, 1, sizeof(cfg), f);
    fflush(f);
    fclose(f);
    return (written == sizeof(cfg));
}

void i18n_init(void) {
    /* 1. Prioridade maxima: preferencia salva pelo usuario em /data/PSgba/config.bin */
    if (i18n_load_config()) {
        return;
    }

    /* 2. Primeira inicializacao: deteccao automatica via Orbis OS SystemService */
    int32_t lang_code = -1;
    int32_t ret = sceSystemServiceGetSystemParamInt(SCE_SYSTEM_SERVICE_PARAM_ID_LANG, &lang_code);
    if (ret == 0 &&
        (lang_code == SCE_SYSTEM_SERVICE_LANGUAGE_PORTUGUESE_PT ||
         lang_code == SCE_SYSTEM_SERVICE_LANGUAGE_PORTUGUESE_BR)) {
        g_current_language = LANG_PT_BR;
    } else {
        g_current_language = LANG_EN_US;
    }
}

AppLanguage i18n_get_language(void) {
    return g_current_language;
}

void i18n_set_language(AppLanguage lang) {
    if ((unsigned int)lang < (unsigned int)LANG_COUNT) {
        g_current_language = lang;
        i18n_save_config();
    }
}

void i18n_cycle_language(int delta) {
    int count = (int)LANG_COUNT;
    int next = ((int)g_current_language + delta) % count;
    if (next < 0) {
        next += count;
    }
    i18n_set_language((AppLanguage)next);
}

