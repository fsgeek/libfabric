#include "rstream.h"

static int rstream_fabric_close(fid_t fid)
{
	struct rstream_fabric *rstream_fabric =
		container_of(fid, struct rstream_fabric,
		util_fabric.fabric_fid.fid);
	int ret;

	ret = fi_close(&rstream_fabric->msg_fabric->fid);
	if (ret)
		return ret;

	ret = ofi_fabric_close(&rstream_fabric->util_fabric);
	if (ret)
		return ret;

	free(rstream_fabric);
	return 0;
}

static int rstream_control(struct fid *fid, int command, void *arg)
{
	return -FI_ENOSYS;
}

static struct fi_ops rstream_fabric_fi_ops = {
	.size = sizeof(struct fi_ops),
	.close = rstream_fabric_close,
	.bind = fi_no_bind,
	.control = rstream_control,
	.ops_open = fi_no_ops_open,
};

static struct fi_ops_fabric rstream_fabric_ops = {
	.size = sizeof(struct fi_ops_fabric),
	.domain = rstream_domain_open,
	.passive_ep = rstream_passive_ep,
	.eq_open = rstream_eq_open,
	.wait_open = fi_no_wait_open,
	.trywait = ofi_trywait
};

int rstream_fabric_open(struct fi_fabric_attr *attr, struct fid_fabric **fabric,
			   void *context)
{
	struct rstream_fabric *rstream_fabric;
	int ret;
	struct fi_info *info = NULL;

	rstream_fabric = calloc(1, sizeof(*rstream_fabric));
	if (!rstream_fabric)
		return -FI_ENOMEM;

	ret = ofi_fabric_init(&rstream_prov, &rstream_fabric_attr, attr,
			&rstream_fabric->util_fabric, context);
	if (ret)
		goto err1;

	ret = ofi_get_core_info_fabric(&rstream_prov, attr, &info);
	if (ret) {
		FI_WARN(&rstream_prov, FI_LOG_FABRIC, "core info failed\n");
		ret = -FI_EINVAL;
		goto err1;
	}

	ret = fi_fabric(info->fabric_attr, &rstream_fabric->msg_fabric, context);
	if (ret) {
		FI_WARN(&rstream_prov, FI_LOG_FABRIC, "fi_fabric failed\n");
		ret = -FI_EINVAL;
		goto err1;
	}

	*fabric = &rstream_fabric->util_fabric.fabric_fid;
	(*fabric)->fid.ops = &rstream_fabric_fi_ops;
	(*fabric)->ops = &rstream_fabric_ops;

	fi_freeinfo(info);
	return 0;
err1:
	free(rstream_fabric);
	if (info)
		fi_freeinfo(info);

	return ret;
}
