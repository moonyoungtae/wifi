

function ret = parse_log(filename)

    endian_format = 'ieee-le';
    min_pkt_len = 165;

    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % Read file and check the length %
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    f = fopen(filename, 'rb');

    if (f < 0)
        error('Failed to open file: %s', filename);
        return;
    end

    fstatus = fseek(f, 0, 'eof');

    if fstatus ~= 0
        [msg, errno] = ferror(f);
        error('Error %d in seeking: %s', errno, msg);
        fclose(f);
        return;
    end

    flen = ftell(f);
    fstatus = fseek(f, 0, 'bof');

    if fstatus ~= 0
        [msg, errno] = ferror(f);
        error('Error %d in seeking: %s', errno, msg);
        fclose(f);
        return;
    end

    %%%%%%%%%%%%
    % Read CSI %
    %%%%%%%%%%%%

    ret = cell(ceil(flen / min_pkt_len), 1);
    cur = 0;
    count = 0;

    while cur < (flen - 4)

        % Read Block Length

        block_len = fread(f, 1, 'uint16', 0, endian_format);
        cur = cur + 2;

        if (cur + block_len) > flen
            break;
        end

        % Read CSI Matrix Data

        csi_mat.timestamp = fread(f, 1, 'uint64', 0, [endian_format '.l64']);
        csi_mat.csi_len = fread(f, 1, 'uint16', 0, endian_format);
        csi_mat.channel = fread(f, 1, 'uint16', 0, endian_format);
        csi_mat.phy_err = fread(f, 1, 'uint8=>int');
        csi_mat.noise = fread(f, 1, 'uint8=>int');
        csi_mat.rate = fread(f, 1, 'uint8=>int');
        csi_mat.bandwidth = fread(f, 1, 'uint8=>int');
        csi_mat.nc = fread(f, 1, 'uint8=>int');
        csi_mat.nr = fread(f, 1, 'uint8=>int');
        csi_mat.nt = fread(f, 1, 'uint8=>int');
        csi_mat.rssi = fread(f, 1, 'uint8=>int');
        csi_mat.rssi1 = fread(f, 1, 'uint8=>int');
        csi_mat.rssi2 = fread(f, 1, 'uint8=>int');
        csi_mat.rssi3 = fread(f, 1, 'uint8=>int');
        csi_mat.payload_len = fread(f, 1, 'uint16', 0, endian_format);
        cur = cur + 25;

        % Read CSI Data

        if csi_mat.csi_len > 0
            csi_buf = fread(f, csi_mat.csi_len, 'uint8=>uint8');
            csi_mat.csi = parse_csi(csi_buf, csi_mat.nr, csi_mat.nt, csi_mat.nc);
            csi_mat.csi = permute(csi_mat.csi, [2 1 3]); % [nt, nr, nc]
            cur = cur + csi_mat.csi_len;
        else
            csi_mat.csi = 0;
        end

        % Read Payload

        if csi_mat.payload_len > 0
            csi_matrix.payload = fread(f, csi_mat.payload_len, 'uint8=>uint8');
            cur = cur + csi_mat.payload_len;
        else
            csi_matrix.payload = 0;
        end

        % Save to the return array

        count = count + 1;
        ret{count} = csi_mat;

        if (cur + min_pkt_len > flen)
            break;
        end

    end

    %%%%%%%%%%
    % Return %
    %%%%%%%%%%

    if (count > 1)
        ret = ret(1:count);
    else
        ret = ret(1);
    end

    fclose(f);
    fprintf("Parsed %d packets.\n", count);

end
