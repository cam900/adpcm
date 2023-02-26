project(SUPERCTR_ADPCM LANGUAGES C)

set(SOURCES
	"adpcm.c"

	"bs_codec.c"
	"oki_codec.c"
	"yma_codec.c"
	"ymb_codec.c"
	"ymz_codec.c"
	"ima_codec.c"
)
add_executable(${PROJECT_NAME} ${SOURCES})