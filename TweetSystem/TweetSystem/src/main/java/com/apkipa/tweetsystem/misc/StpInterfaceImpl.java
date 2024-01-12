package com.apkipa.tweetsystem.misc;

import cn.dev33.satoken.stp.StpInterface;
import cn.dev33.satoken.stp.StpUtil;
import com.apkipa.tweetsystem.model.ERole;
import com.apkipa.tweetsystem.repository.UserRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import java.util.ArrayList;
import java.util.List;

@Component
public class StpInterfaceImpl implements StpInterface {
    @Autowired
    UserRepository userRepository;

    @Override
    public List<String> getPermissionList(Object loginId, String loginType) {
        var roles = getRoleList(loginId, loginType);
        //if (roles.contains(ERole.ROLE_ADMIN))
        return new ArrayList<>();
    }

    @Override
    public List<String> getRoleList(Object loginId, String loginType) {
        var user = userRepository.findNullable(Long.parseLong(String.valueOf(loginId)));
        var roles = new ArrayList<String>();
        switch (user.role()) {
            case ROLE_ADMIN:
                roles.add(ERole.ROLE_ADMIN.toString());
            case ROLE_REVIEW:
                roles.add(ERole.ROLE_REVIEW.toString());
            case ROLE_USER:
                roles.add(ERole.ROLE_USER.toString());
        }
        return roles;
    }
}
