#include "pando_subdevice.h"
#include "c_types.h"
#include "user_interface.h"
#include "gateway/pando_channel.h"
#include "protocol/platform_functions.h"
#include "protocol/sub_device_protocol_tool.h"
#include "pando_object.h"

#define CMD_QUERY_STATUS (65528)


static void ICACHE_FLASH_ATTR
decode_data(struct sub_device_buffer *device_buffer)
{
	struct pando_property data_body;
	uint8_t *buf_end = device_buffer->buffer + device_buffer->buffer_length;
	uint16_t tlv_type, tlv_length;
	uint8_t *value = NULL;

	struct TLV *object_param = get_sub_device_property(device_buffer, &data_body);
	pd_printf("count: %d\n", data_body.params->count);

	pando_object* obj = find_pando_object(data_body.property_num);

	if( NULL == obj )
	{
		PRINTF("object [%d] not found in list\n", data_body.property_num);
	}

	obj->unpack(object_param,  data_body.params->count);


}

static void ICACHE_FLASH_ATTR
send_current_status()
{
	struct sub_device_buffer* data_buffer;

	pando_object* obj = NULL;
	pando_objects_iterator* it = create_pando_objects_iterator();
	while(obj = pando_objects_iterator_next(it)){
		struct TLVs* params =  obj->pack();
		if(0 == it->cur)
		{
			data_buffer = create_data_package(obj->no, 0, params);
			if( NULL == data_buffer)
			{
				PRINTF("Create data package failed.");
			}

		}
		int ret = add_next_property(data_buffer, obj->no, params);

		if (ret != 0)
		{
			PRINTF("add_next_property failed.");
		}
		delete_params_block(params);

	}
	delete_pando_objects_iterator(it);

	channel_send_to_device(PANDO_CHANNEL_PORT_1, data_buffer->buffer, data_buffer->buffer_length);
	show_package(data_buffer->buffer, data_buffer->buffer_length);

	delete_device_package(data_buffer);
}

static void ICACHE_FLASH_ATTR
decode_command(struct sub_device_buffer *device_buffer)
{
	struct pando_command cmd_body;
	struct TLV *cmd_param = get_sub_device_command(device_buffer, &cmd_body);
	if(CMD_QUERY_STATUS == cmd_body.command_id)
	{
		PRINTF("receive a get request\n");
	    subdevice_data_send();
	}
}

void ICACHE_FLASH_ATTR
pando_subdevice_recv(uint8_t * buffer, uint16_t length)
{
	if(NULL == buffer)
	{
		return;
	}

	struct sub_device_buffer *device_buffer = (struct sub_device_buffer *)pd_malloc(sizeof(struct sub_device_buffer));
	device_buffer->buffer_length = length;
	device_buffer->buffer = (uint8 *)pd_malloc(length);

	pd_memcpy(device_buffer->buffer, buffer, length);

	uint16 payload_type = get_sub_device_payloadtype(device_buffer);

	switch (payload_type) {
	case PAYLOAD_TYPE_DATA:
		decode_data(device_buffer);
		break;
	case PAYLOAD_TYPE_COMMAND:
		decode_command(device_buffer);
		break;
	default:
		PRINTF("unsuported paload type : %d", payload_type);
		break;
	}

	delete_device_package(device_buffer);
}
