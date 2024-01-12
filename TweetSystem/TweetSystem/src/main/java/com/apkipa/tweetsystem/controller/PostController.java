package com.apkipa.tweetsystem.controller;

import cn.dev33.satoken.annotation.SaCheckLogin;
import cn.dev33.satoken.annotation.SaCheckRole;
import cn.dev33.satoken.stp.StpUtil;
import com.apkipa.tweetsystem.model.*;
import com.apkipa.tweetsystem.repository.PostRepository;
import com.apkipa.tweetsystem.repository.UserRelationshipRepository;
import com.apkipa.tweetsystem.repository.UserRepository;
import com.apkipa.tweetsystem.result.JResp;
import org.babyfish.jimmer.Input;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.sql.Timestamp;
import java.time.Instant;

@RestController
@RequestMapping("/api/v1/post")
public class PostController {
    @Autowired
    PostRepository postRepository;
    @Autowired
    UserRepository userRepository;
    @Autowired
    UserRelationshipRepository userRelationshipRepository;

    @SaCheckLogin
    @GetMapping("/list")
    public JResp<?> list(@RequestParam Long user_id) {
        var thisUserId = StpUtil.getLoginIdAsLong();
        if (userRelationshipRepository.isBlockedByUser(thisUserId, user_id)) {
            return JResp.err(-101, "由于对方设置，你无法查看推文");
        }
        if (thisUserId == user_id) {
            var posts = postRepository.listNotRejectedByUserId(user_id,
                    PostFetcher.$
                            .allScalarFields()
                            .user(UserFetcher.$.name().nickname())
                            .likes(LikeFetcher.$.user(UserFetcher.$))
                            .collections(CollectionFetcher.$.user())
                            .replyPost(PostFetcher.$
                                    .allScalarFields()
                                    .user(UserFetcher.$.name().nickname())
                                    .likes(LikeFetcher.$.user(UserFetcher.$))
                                    .collections(CollectionFetcher.$.user())
                                    .replyPosts()
                            )
                            .replyPosts()
                            .forwardPost(PostFetcher.$
                                    .allScalarFields()
                                    .user(UserFetcher.$.name().nickname())
                                    .likes(LikeFetcher.$.user(UserFetcher.$))
                                    .collections(CollectionFetcher.$.user())
                                    .replyPosts()
                            )
            );
            return JResp.ok(posts);
        }
        var posts = postRepository.listPassedByUserId(user_id,
                PostFetcher.$
                        .allScalarFields()
                        .user(UserFetcher.$.name().nickname())
                        .likes(LikeFetcher.$.user(UserFetcher.$))
                        .collections(CollectionFetcher.$.user())
                        .replyPost(PostFetcher.$
                                .allScalarFields()
                                .user(UserFetcher.$.name().nickname())
                                .likes(LikeFetcher.$.user(UserFetcher.$))
                                .collections(CollectionFetcher.$.user())
                                .replyPosts()
                        )
                        .replyPosts()
                        .forwardPost(PostFetcher.$
                                .allScalarFields()
                                .user(UserFetcher.$.name().nickname())
                                .likes(LikeFetcher.$.user(UserFetcher.$))
                                .collections(CollectionFetcher.$.user())
                                .replyPosts()
                        )
        );
        return JResp.ok(posts);
    }

    @SaCheckLogin
    @GetMapping("/view")
    public JResp<?> view(@RequestParam Long post_id) {
        var thisUserId = StpUtil.getLoginIdAsLong();
        var postOpt = postRepository.findById(post_id,
                PostFetcher.$
                        .allScalarFields()
                        .user(UserFetcher.$.name().nickname())
                        .likes(LikeFetcher.$.user(UserFetcher.$))
                        .collections(CollectionFetcher.$.user())
                        .replyPost(PostFetcher.$
                                .allScalarFields()
                                .user(UserFetcher.$.name().nickname())
                                .likes(LikeFetcher.$.user(UserFetcher.$))
                                .collections(CollectionFetcher.$.user())
                                .replyPosts()
                        )
                        .forwardPost(PostFetcher.$
                                .allScalarFields()
                                .user(UserFetcher.$.name().nickname())
                                .likes(LikeFetcher.$.user(UserFetcher.$))
                                .collections(CollectionFetcher.$.user())
                                .replyPosts()
                        )
                        .replyPosts(PostFetcher.$
                                .allScalarFields()
                                .user(UserFetcher.$.name().nickname())
                                .likes(LikeFetcher.$.user(UserFetcher.$))
                                .collections(CollectionFetcher.$.user())
                                .replyPost()
                                .replyPosts()
                        )
        );
        if (postOpt.isEmpty()) {
            return JResp.err(-1, "推文无效");
        }
        var post = postOpt.get();
        if (userRelationshipRepository.isBlockedByUser(thisUserId, post.user().id())) {
            return JResp.err(-101, "由于对方设置，你无法查看推文");
        }
        return JResp.ok(post);
    }

    @SaCheckLogin
    @PostMapping("/create")
    public JResp<?> create(
            @RequestParam String content,
            @RequestParam(required = false) Long reply_id
    ) {
        Post replyPost;
        if (reply_id != null) {
            var replyPostOpt = postRepository.findById(reply_id);
            if (replyPostOpt.isEmpty()) {
                return JResp.err(-1, "回复推文无效");
            }
            replyPost = replyPostOpt.get();
        } else {
            replyPost = null;
        }
        var thisUserId = StpUtil.getLoginIdAsLong();
        var thisUser = userRepository.findNullable(thisUserId);
        var post = PostDraft.$.produce(draft -> {
            draft.setAuditState(EAudit.Draft);
            draft.setContent(content);
            draft.setUser(thisUser);
            draft.setPublishTime(Timestamp.from(Instant.now()));
            draft.setReplyPost(replyPost);
            draft.setDeleted(false);
        });
        var postId = postRepository.save(post).id();
        post = postRepository.findNullable(postId, PostFetcher.$
                .allScalarFields()
        );
        return JResp.ok(post);
    }

    @SaCheckLogin
    @PostMapping("/create-forward")
    public JResp<?> createForward(
            @RequestParam Long post_id
    ) {
        var thisUserId = StpUtil.getLoginIdAsLong();
        var thisUser = userRepository.findNullable(thisUserId);
        var forwardPostOpt = postRepository.findById(post_id);
        if (forwardPostOpt.isEmpty()) {
            return JResp.err(-1, "转发推文无效");
        }
        var post = PostDraft.$.produce(draft -> {
            draft.setAuditState(EAudit.Passed);
            draft.setContent("");
            draft.setUser(thisUser);
            draft.setPublishTime(Timestamp.from(Instant.now()));
            draft.setForwardPost(forwardPostOpt.get());
            draft.setDeleted(false);
        });
        var postId = postRepository.save(post).id();
        post = postRepository.findNullable(post_id, PostFetcher.$
                .allScalarFields()
        );
        return JResp.ok(post);
    }

    @SaCheckLogin
    @PostMapping("/delete")
    public JResp<?> delete(
            @RequestParam Long id
    ) {
        var thisUserId = StpUtil.getLoginIdAsLong();
        var postOpt = postRepository.findById(id);
        if (postOpt.isEmpty()) {
            return JResp.err(-1, "推文无效");
        }
        var oldPost = postOpt.get();
        if (oldPost.user().id() != thisUserId) {
            return JResp.err(-1, "无修改此推文的权限");
        }
        //postRepository.delete(oldPost);
        postRepository.save(PostDraft.$.produce(oldPost, draft -> {
            draft.setDeleted(true);
        }));
        return JResp.ok(null);
    }

    @SaCheckLogin
    @PostMapping("/update")
    public JResp<?> update(
            @RequestParam Long id,
            @RequestParam String content
    ) {
        var thisUserId = StpUtil.getLoginIdAsLong();
        var postOpt = postRepository.findById(id);
        if (postOpt.isEmpty()) {
            return JResp.err(-1, "推文无效");
        }
        var oldPost = postOpt.get();
        if (oldPost.user().id() != thisUserId) {
            return JResp.err(-1, "无修改此推文的权限");
        }
        var oldAudit = oldPost.auditState();
        if (oldAudit != EAudit.Draft && oldAudit != EAudit.Rejected) {
            return JResp.err(-1, "此推文所处的审核阶段不允许修改内容");
        }
        var post = PostDraft.$.produce(oldPost, draft -> {
            draft.setContent(content);
        });
        postRepository.save(post);
        return JResp.ok(null);
    }

    @SaCheckLogin
    @PostMapping("/commit-audit")
    public JResp<?> commitAudit(
            @RequestParam Long id
    ) {
        var thisUserId = StpUtil.getLoginIdAsLong();
        var postOpt = postRepository.findById(id);
        if (postOpt.isEmpty()) {
            return JResp.err(-1, "推文无效");
        }
        var oldPost = postOpt.get();
        if (oldPost.user().id() != thisUserId) {
            return JResp.err(-1, "无修改此推文的权限");
        }
        var oldAudit = oldPost.auditState();
        if (oldAudit != EAudit.Draft && oldAudit != EAudit.Rejected) {
            return JResp.err(-1, "此推文所处的审核阶段不允许提交审核");
        }
        var post = PostDraft.$.produce(oldPost, draft -> {
            draft.setAuditState(EAudit.InProgress);
        });
        postRepository.save(post);
        return JResp.ok(null);
    }

    @SaCheckRole("ROLE_REVIEW")
    @GetMapping("/audit-list-pending")
    public JResp<?> auditListPending() {
        var posts = postRepository.listAllInProgress();
        return JResp.ok(posts);
    }

    @SaCheckRole("ROLE_REVIEW")
    @PostMapping("/audit-accept")
    public JResp<?> auditAccept(@RequestParam Long post_id) {
        var postOpt = postRepository.findById(post_id);
        if (postOpt.isEmpty()) {
            return JResp.err(-1, "推文无效");
        }
        var oldPost = postOpt.get();
        var oldAudit = oldPost.auditState();
        if (oldAudit != EAudit.InProgress) {
            return JResp.err(-1, "此推文所处的审核阶段不允许批准");
        }
        var post = PostDraft.$.produce(oldPost, draft -> {
            draft.setAuditState(EAudit.Passed);
        });
        postRepository.save(post);
        return JResp.ok(null);
    }

    @SaCheckRole("ROLE_REVIEW")
    @PostMapping("/audit-reject")
    public JResp<?> auditReject(@RequestParam Long post_id) {
        var postOpt = postRepository.findById(post_id);
        if (postOpt.isEmpty()) {
            return JResp.err(-1, "推文无效");
        }
        var oldPost = postOpt.get();
        var oldAudit = oldPost.auditState();
        if (oldAudit != EAudit.InProgress) {
            return JResp.err(-1, "此推文所处的审核阶段不允许批准");
        }
        var post = PostDraft.$.produce(oldPost, draft -> {
            draft.setAuditState(EAudit.Rejected);
        });
        postRepository.save(post);
        return JResp.ok(null);
    }

    @SaCheckLogin
    @GetMapping("/search")
    public JResp<?> search(@RequestParam String content) {
        var thisUserId = StpUtil.getLoginIdAsLong();
        var posts = postRepository.searchPassedByContent(content,
                thisUserId,
                PostFetcher.$
                        .allScalarFields()
                        .user(UserFetcher.$.name().nickname())
                        .likes(LikeFetcher.$.user(UserFetcher.$))
                        .collections(CollectionFetcher.$.user())
                        .replyPosts()
        );
        //posts = posts.stream().filter();
        return JResp.ok(posts);
    }
}
