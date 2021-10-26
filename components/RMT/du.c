/*
 * du.c
 *
 *  Created on: 30 sept. 2021
 *      Author: i2CAT
 */




#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#include "RMT.h"
#include "du.h"
#include "pci.h"
#include "IPCP.h"
#include "BufferManagement.h"

BaseType_t xDuDestroy(struct du_t * pxDu)
{
	/*If there is an NetworkBuffer then release and */
	if (pxDu->pxNetworkBuffer)
		vReleaseNetworkBuffer( pxDu->pxNetworkBuffer->pucEthernetBuffer );

	vPortFree(pxDu);

	return pdTRUE;
}



BaseType_t xDuDecap(struct du_t * pxDu)
{
	pduType_t xType;
	pci_t * pxPciTmp;
	size_t uxPciLen;
	NetworkBufferDescriptor_t * pxNewBuffer;
	uint8_t * pucPtr;
	size_t xBufferSize;
	   

	/* Extract PCI from buffer*/
	pxPciTmp = vCastPointerTo_pci_t(pxDu->pxNetworkBuffer->pucEthernetBuffer);
	
	xType = pxPciTmp->xType;
	if (unlikely(!pdu_type_is_ok(xType))) {
		ESP_LOGE(TAG_RMT, "Could not decap DU. Type is not ok");
		return pdFALSE;
	}

	uxPciLen = (size_t )(14); /* PCI defined static for this initial stage = 14Bytes*/

	xBufferSize = pxDu->pxNetworkBuffer->xDataLength - uxPciLen - pxPciTmp->xPduLen;

	pxNewBuffer = pxGetNetworkBufferWithDescriptor( xBufferSize, ( TickType_t ) 0U );
	pxNewBuffer->xDataLength = xBufferSize;

	pucPtr = (uint8_t *)pxPciTmp + 14;

	memcpy(pxNewBuffer, pucPtr,xBufferSize);


	pxDu->pxPci->ucVersion = pxPciTmp->ucVersion;
	pxDu->pxPci->xSource = pxPciTmp->xSource;
	pxDu->pxPci->xDestination = pxPciTmp->xDestination;
	pxDu->pxPci->xFlags = pxPciTmp->xFlags;
	pxDu->pxPci->xPduLen = pxPciTmp->xPduLen;
	pxDu->pxPci->xSequenceNumber = pxPciTmp->xSequenceNumber;
	pxDu->pxPci->xType = pxPciTmp->xType;
	pxDu->pxPci->connectionId_t = pxPciTmp->connectionId_t;

	vReleaseNetworkBufferAndDescriptor(pxDu->pxNetworkBuffer);
	pxDu->pxNetworkBuffer = pxNewBuffer;

	return pdTRUE;
}

ssize_t xDuDataLen(const struct du_t * pxDu)
{
	if (pxDu->pxPci->xPduLen != pxDu->pxNetworkBuffer->xDataLength) /* up direction */
		return pxDu->pxNetworkBuffer->xDataLength;
	return (pxDu->pxNetworkBuffer->xDataLength - pxDu->pxPci->xPduLen); /* down direction */
}


