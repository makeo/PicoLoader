MEMORY
{
    HEADER(r) : ORIGIN = ORIGIN(FLASH) + LENGTH(FLASH) + ALIGN(66k, 4k) - 66k, LENGTH = 66k
    PAYLOAD(r) : ORIGIN = ORIGIN(HEADER) + LENGTH(HEADER), LENGTH = 2048k - LENGTH(FLASH) - LENGTH(HEADER)
}

SECTIONS
{
    .header : {
        KEEP(*(.header))
    } > HEADER
}

data_header = ORIGIN(HEADER);
data_payload = ORIGIN(PAYLOAD);
