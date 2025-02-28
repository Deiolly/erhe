﻿#include "erhe/xr/xr_session.hpp"
#include "erhe/gl/enum_string_functions.hpp"
#include "erhe/toolkit/profile.hpp"
#include "erhe/toolkit/verify.hpp"
#include "erhe/toolkit/window.hpp"
#include "erhe/xr/xr_instance.hpp"
#include "erhe/xr/xr.hpp"
#include "erhe/xr/xr_swapchain_image.hpp"

#ifdef _WIN32
#   define GLFW_EXPOSE_NATIVE_WIN32 1
#   define GLFW_EXPOSE_NATIVE_WGL   1
#endif
#include <GLFW/glfw3.h>

#ifdef _WIN32
#   include <GLFW/glfw3native.h>
#endif

#ifdef _WIN32
#   include <unknwn.h>
#   define XR_USE_PLATFORM_WIN32      1
#endif

//#if defined(_MSC_VER) && !defined(__clang__)
//#   pragma warning(push)
//#   pragma warning(disable : 26812) // The enum type is unscoped. Prefer ‘enum class’ over ‘enum’ (Enum.3).
//#endif

#ifdef linux
#   define XR_USE_PLATFORM_LINUX      1
#endif

//#define XR_USE_GRAPHICS_API_VULKAN 1
#define XR_USE_GRAPHICS_API_OPENGL 1
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

//#if defined(_MSC_VER) && !defined(__clang__)
//#   pragma warning(pop)
//#endif

namespace erhe::xr {

Xr_session::Xr_session(
    Xr_instance&                   instance,
    erhe::toolkit::Context_window& context_window
)
    : m_instance              {instance}
    , m_context_window        {context_window}
    , m_xr_session            {XR_NULL_HANDLE}
    , m_swapchain_color_format{gl::Internal_format::srgb8_alpha8}
    , m_swapchain_depth_format{gl::Internal_format::depth24_stencil8}
    , m_xr_reference_space    {XR_NULL_HANDLE}
    //, m_xr_session_state      {XR_SESSION_STATE_VISIBLE}
    , m_xr_frame_state        {
        XR_TYPE_FRAME_STATE,
        nullptr,
        0,
        0,
        XR_FALSE
    }
{
    ERHE_PROFILE_FUNCTION

    log_xr.trace("{}\n", __func__);

    if (instance.get_xr_instance() == XR_NULL_HANDLE)
    {
        log_xr.error("No XR Instance\n");
        return;
    }

    m_xr_views.resize(instance.get_xr_view_configuration_views().size());

    if (!create_session())
    {
        return;
    }
    if (!enumerate_swapchain_formats())
    {
        return;
    }
    if (!enumerate_reference_spaces())
    {
        return;
    }
    if (!create_swapchains())
    {
        return;
    }
    if (!create_reference_space())
    {
        return;
    }
    if (!begin_session())
    {
        return;
    }
    if (!attach_actions())
    {
        return;
    }
    if (!create_hand_tracking())
    {
        return;
    }
}

auto Xr_session::create_session() -> bool
{
    ERHE_PROFILE_FUNCTION

    log_xr.trace("{}\n", __func__);

    const auto xr_instance = m_instance.get_xr_instance();
    ERHE_VERIFY(xr_instance != XR_NULL_HANDLE);

    XrGraphicsRequirementsOpenGLKHR xr_graphics_requirements_opengl{
        .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR,
        .next = nullptr
    };

    ERHE_XR_CHECK(
        m_instance.xrGetOpenGLGraphicsRequirementsKHR(
            xr_instance,
            m_instance.get_xr_system_id(),
            &xr_graphics_requirements_opengl
        )
    );

    GLFWwindow* const glfw_window = m_context_window.get_glfw_window();
    if (glfw_window == nullptr)
    {
        log_xr.error("No GLFW window\n");
        return false;
    }

#ifdef _WIN32
    XrGraphicsBindingOpenGLWin32KHR graphics_binding_opengl_win32{
        .type  = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
        .next  = nullptr,
        .hDC   = GetDC(glfwGetWin32Window(glfw_window)),
        .hGLRC = glfwGetWGLContext(glfw_window)
    };

    XrSessionCreateInfo session_create_info{
        .type        = XR_TYPE_SESSION_CREATE_INFO,
        .next        = reinterpret_cast<const XrBaseInStructure*>(&graphics_binding_opengl_win32),
        .createFlags = 0,
        .systemId    = m_instance.get_xr_system_id()
    };

    check_gl_context_in_current_in_this_thread();
    ERHE_XR_CHECK(xrCreateSession(xr_instance, &session_create_info, &m_xr_session));
#endif

#ifdef linux
    // TODO
    abort();
#endif

    if (m_xr_session == XR_NULL_HANDLE)
    {
        log_xr.error("xrCreateSession() created XR_NULL_HANDLE session\n");
        return false;
    }

    return true;
}

Xr_session::~Xr_session()
{
    log_xr.trace("{}\n", __func__);

    if (m_instance.xrDestroyHandTrackerEXT != nullptr)
    {
        if (m_hand_tracker_left.hand_tracker != XR_NULL_HANDLE)
        {
            m_instance.xrDestroyHandTrackerEXT(m_hand_tracker_left.hand_tracker);
        }
        if (m_hand_tracker_right.hand_tracker != XR_NULL_HANDLE)
        {
            m_instance.xrDestroyHandTrackerEXT(m_hand_tracker_right.hand_tracker);
        }
    }

    m_view_swapchains.clear();

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return;
    }

    check(xrEndSession    (m_xr_session));

    check_gl_context_in_current_in_this_thread();
    check(xrDestroySession(m_xr_session));
}

auto Xr_session::get_xr_session() const -> XrSession
{
    return m_xr_session;
}

auto Xr_session::get_xr_reference_space() const -> XrSpace
{
    return m_xr_reference_space;
}

auto Xr_session::get_xr_frame_state() const -> const XrFrameState&
{
    return m_xr_frame_state;
}

int color_format_score(const gl::Internal_format image_format)
{
    switch (image_format)
    {
        //using enum gl::Internal_format;
        case gl::Internal_format::rgba8:          return 1;
        case gl::Internal_format::srgb8_alpha8:   return 2;
        case gl::Internal_format::rgb10_a2:       return 3;
        case gl::Internal_format::r11f_g11f_b10f: return 4;
        case gl::Internal_format::rgba16f:        return 5;
        default:                                  return 0;
    }
}

auto Xr_session::enumerate_swapchain_formats() -> bool
{
    ERHE_PROFILE_FUNCTION

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return false;
    }

    uint32_t swapchain_format_count{0};

    ERHE_XR_CHECK(xrEnumerateSwapchainFormats(m_xr_session, 0, &swapchain_format_count, nullptr));

    std::vector<int64_t> swapchain_formats(swapchain_format_count);

    ERHE_XR_CHECK(
        xrEnumerateSwapchainFormats(
            m_xr_session,
            swapchain_format_count,
            &swapchain_format_count,
            swapchain_formats.data()
        )
    );

    log_xr.info("Swapchain formats:\n");
    int best_color_format_score{0};
    m_swapchain_color_format = gl::Internal_format::srgb8_alpha8;
    for (const auto swapchain_format : swapchain_formats)
    {
        const auto gl_internal_format = static_cast<gl::Internal_format>(swapchain_format);
        log_xr.info("    {}\n", gl::c_str(gl_internal_format));
        const int color_score = color_format_score(gl_internal_format);
        if (color_score > best_color_format_score)
        {
            best_color_format_score = color_score;
            m_swapchain_color_format = gl_internal_format;
        }
    }
    log_xr.info("Selected swapchain color format {}\n", gl::c_str(m_swapchain_color_format));

    return true;
}

auto Xr_session::enumerate_reference_spaces() -> bool
{
    ERHE_PROFILE_FUNCTION

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return false;
    }

    uint32_t reference_space_type_count{0};
    ERHE_XR_CHECK(
        xrEnumerateReferenceSpaces(
            m_xr_session,
            0,
            &reference_space_type_count,
            nullptr
        )
    );

    m_xr_reference_space_types.resize(reference_space_type_count);
    ERHE_XR_CHECK(
        xrEnumerateReferenceSpaces(
            m_xr_session,
            reference_space_type_count,
            &reference_space_type_count,
            m_xr_reference_space_types.data()
        )
    );
    log_xr.info("Reference space types:\n");
    for (const auto reference_space_type : m_xr_reference_space_types)
    {
        log_xr.info("    {}\n", c_str(reference_space_type));
    }

    return true;
}

auto Xr_session::create_swapchains() -> bool
{
    ERHE_PROFILE_FUNCTION

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return false;
    }

    const auto& views = m_instance.get_xr_view_configuration_views();
    m_view_swapchains.clear();
    for (const auto& view : views)
    {
        const XrSwapchainCreateInfo color_swapchain_create_info
        {
            .type        = XR_TYPE_SWAPCHAIN_CREATE_INFO,
            .next        = nullptr,
            .createFlags = 0,
            .usageFlags  = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
            .format      = static_cast<int64_t>(m_swapchain_color_format),
            .sampleCount = view.recommendedSwapchainSampleCount,
            .width       = view.recommendedImageRectWidth,
            .height      = view.recommendedImageRectHeight,
            .faceCount   = 1,
            .arraySize   = 1,
            .mipCount    = 1
        };

        XrSwapchain color_swapchain{XR_NULL_HANDLE};

        check_gl_context_in_current_in_this_thread();
        ERHE_XR_CHECK(xrCreateSwapchain(m_xr_session, &color_swapchain_create_info, &color_swapchain));

        const XrSwapchainCreateInfo depth_swapchain_create_info
        {
            .type        = XR_TYPE_SWAPCHAIN_CREATE_INFO,
            .next        = nullptr,
            .createFlags = 0,
            //.usageFlags  = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .usageFlags  = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .format      = static_cast<int64_t>(m_swapchain_depth_format),
            .sampleCount = view.recommendedSwapchainSampleCount,
            .width       = view.recommendedImageRectWidth,
            .height      = view.recommendedImageRectHeight,
            .faceCount   = 1,
            .arraySize   = 1,
            .mipCount    = 1
        };

        XrSwapchain depth_swapchain{XR_NULL_HANDLE};

        check_gl_context_in_current_in_this_thread();
        ERHE_XR_CHECK(xrCreateSwapchain(m_xr_session, &depth_swapchain_create_info, &depth_swapchain));
        m_view_swapchains.emplace_back(color_swapchain, depth_swapchain);
    }

    return true;
}

auto Xr_session::create_reference_space() -> bool
{
    ERHE_PROFILE_FUNCTION

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return false;
    }

    XrReferenceSpaceCreateInfo reference_space_create_info{
        .type                 = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
        .next                 = nullptr,
        .referenceSpaceType   = XR_REFERENCE_SPACE_TYPE_LOCAL,
        .poseInReferenceSpace = {
            .orientation = {
                .x = 0.0f,
                .y = 0.0f,
                .z = 0.0f,
                .w = 1.0f
            },
            .position = {
                .x = 0.0f,
                .y = 0.0f,
                .z = 0.0f
            }
        }
    };

    ERHE_XR_CHECK(xrCreateReferenceSpace(m_xr_session, &reference_space_create_info, &m_xr_reference_space));

    return true;
}

auto Xr_session::begin_session() -> bool
{
    ERHE_PROFILE_FUNCTION

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return false;
    }

    XrSessionBeginInfo session_begin_info {
        .type                         = XR_TYPE_SESSION_BEGIN_INFO,
        .next                         = nullptr,
        .primaryViewConfigurationType = m_instance.get_xr_view_configuration_type()
    };

    ERHE_XR_CHECK(xrBeginSession(m_xr_session, &session_begin_info));

    // m_instance.set_environment_depth_estimation(m_session, true);

    return true;
}

auto Xr_session::attach_actions() -> bool
{
    ERHE_PROFILE_FUNCTION

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return false;
    }

    const XrSessionActionSetsAttachInfo session_action_sets_attach_info{
        .type            = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
        .next            = nullptr,
        .countActionSets = 1,
        .actionSets      = &m_instance.actions.action_set
    };
    ERHE_XR_CHECK(xrAttachSessionActionSets(m_xr_session, &session_action_sets_attach_info));

    const XrActionSpaceCreateInfo action_space_create_info
    {
        .type              = XR_TYPE_ACTION_SPACE_CREATE_INFO,
        .next              = nullptr,
        .action            = m_instance.actions.aim_pose,
        .subactionPath     = XR_NULL_PATH,
        .poseInActionSpace = {
            .orientation = {
                .x = 0.0f,
                .y = 0.0f,
                .z = 0.0f,
                .w = 1.0f
            },
            .position = {
                .x    = 0.0f,
                .y    = 0.0f,
                .z    = 0.0f,
            }
        }
    };
    ERHE_XR_CHECK(xrCreateActionSpace(m_xr_session, &action_space_create_info, &m_instance.actions.aim_pose_space));

    return true;
}

auto Xr_session::create_hand_tracking() -> bool
{
    ERHE_PROFILE_FUNCTION

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return false;
    }

    if (m_instance.xrCreateHandTrackerEXT == nullptr)
    {
        return false;
    }

    const XrHandTrackerCreateInfoEXT left_hand{
        .type         = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
        .next         = nullptr,
        .hand         = XR_HAND_LEFT_EXT,
        .handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT
    };
    const XrHandTrackerCreateInfoEXT right_hand{
        .type         = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
        .next         = nullptr,
        .hand         = XR_HAND_RIGHT_EXT,
        .handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT
    };

    ERHE_XR_CHECK(m_instance.xrCreateHandTrackerEXT(m_xr_session, &left_hand,  &m_hand_tracker_left.hand_tracker));
    ERHE_XR_CHECK(m_instance.xrCreateHandTrackerEXT(m_xr_session, &right_hand, &m_hand_tracker_right.hand_tracker));

    m_hand_tracker_left.velocities = {
        .type            = XR_TYPE_HAND_JOINT_VELOCITIES_EXT,
        .next            = nullptr,
        .jointCount      = XR_HAND_JOINT_COUNT_EXT,
        .jointVelocities = &m_hand_tracker_left.joint_velocities[0]
    };
    m_hand_tracker_left.locations = {
        .type           = XR_TYPE_HAND_JOINT_LOCATIONS_EXT,
        .next           = &m_hand_tracker_left.velocities,
        .isActive       = XR_FALSE,
        .jointCount     = XR_HAND_JOINT_COUNT_EXT,
        .jointLocations = &m_hand_tracker_left.joint_locations[0]
    };

    m_hand_tracker_right.velocities = {
        .type            = XR_TYPE_HAND_JOINT_VELOCITIES_EXT,
        .next            = nullptr,
        .jointCount      = XR_HAND_JOINT_COUNT_EXT,
        .jointVelocities = &m_hand_tracker_right.joint_velocities[0]
    };
    m_hand_tracker_right.locations = {
        .type           = XR_TYPE_HAND_JOINT_LOCATIONS_EXT,
        .next           = &m_hand_tracker_right.velocities,
        .isActive       = XR_FALSE,
        .jointCount     = XR_HAND_JOINT_COUNT_EXT,
        .jointLocations = &m_hand_tracker_right.joint_locations[0]
    };

    update_hand_tracking();
    return true;
}

void Xr_session::update_hand_tracking()
{
    ERHE_PROFILE_FUNCTION

    if (m_instance.xrLocateHandJointsEXT == nullptr)
    {
        return;
    }

#if 0
    XrHandJointsLocateInfoEXT leftHandJointsLocateInfo{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
    XrHandJointLocationsEXT   leftHandLocations       {XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
    XrHandJointLocationsEXT   rightHandLocations      {XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
    XrHandJointVelocitiesEXT  leftHandVelocities      {XR_TYPE_HAND_JOINT_VELOCITIES_EXT};
    XrHandJointVelocitiesEXT  rightHandVelocities     {XR_TYPE_HAND_JOINT_VELOCITIES_EXT};
    XrHandJointLocationEXT    leftHandLocation [XR_HAND_JOINT_COUNT_EXT];
    XrHandJointLocationEXT    rightHandLocation[XR_HAND_JOINT_COUNT_EXT];
    XrHandJointVelocityEXT    leftHandVelocity [XR_HAND_JOINT_COUNT_EXT];
    XrHandJointVelocityEXT    rightHandVelocity[XR_HAND_JOINT_COUNT_EXT];

    leftHandLocations.jointLocations = leftHandLocation;
    leftHandLocations.jointCount     = XR_HAND_JOINT_COUNT_EXT;
    leftHandLocations.isActive       = XR_FALSE;
    leftHandLocations.next           = &leftHandVelocities;
    leftHandVelocities.jointVelocities = leftHandVelocity;
    leftHandVelocities.jointCount      = XR_HAND_JOINT_COUNT_EXT;
    rightHandLocations.jointLocations  = rightHandLocation;
    rightHandLocations.jointCount      = XR_HAND_JOINT_COUNT_EXT;
    rightHandLocations.isActive        = XR_FALSE;
    rightHandLocations.next            = &rightHandVelocities;
    rightHandVelocities.jointVelocities = rightHandVelocity;
    rightHandVelocities.jointCount      = XR_HAND_JOINT_COUNT_EXT;

    leftHandJointsLocateInfo.baseSpace = m_xr_reference_space;
    leftHandJointsLocateInfo.time      = m_xr_frame_state.predictedDisplayTime;

    m_instance.xrLocateHandJointsEXT(m_hand_tracker_left.hand_tracker, &leftHandJointsLocateInfo, &leftHandLocations);
    m_instance.xrLocateHandJointsEXT(m_hand_tracker_right.hand_tracker, &leftHandJointsLocateInfo/*rightHandJointsLocateInfo*/, &rightHandLocations);

    if (leftHandLocations.isActive) {
        printf("Left hand is active\n");
        printf("Left hand wrist position tracked=%s, rotation tracked=%s\n",
            (leftHandLocation[XR_HAND_JOINT_WRIST_EXT].locationFlags & XR_SPACE_LOCATION_POSITION_TRACKED_BIT ? "true" : "false"),
            (leftHandLocation[XR_HAND_JOINT_WRIST_EXT].locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT ? "true" : "false"));
        printf(
            "Left hand wrist position = %f, %f, %f\n",
            leftHandLocation[XR_HAND_JOINT_WRIST_EXT].pose.position.x,
            leftHandLocation[XR_HAND_JOINT_WRIST_EXT].pose.position.y,
            leftHandLocation[XR_HAND_JOINT_WRIST_EXT].pose.position.z
        );
    } else {
        printf("Left hand *Not* tracked\n");
    }

#endif

    const XrHandJointsLocateInfoEXT hand_joints_locate_info{
        .type      = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
        .next      = nullptr,
        .baseSpace = m_xr_reference_space,
        .time      = m_xr_frame_state.predictedDisplayTime
    };

    if (m_hand_tracker_left.hand_tracker != XR_NULL_HANDLE)
    {
        check(
            m_instance.xrLocateHandJointsEXT(
                m_hand_tracker_left.hand_tracker,
                &hand_joints_locate_info,
                &m_hand_tracker_left.locations
            )
        );
    }
    if (m_hand_tracker_right.hand_tracker != XR_NULL_HANDLE)
    {
        check(
            m_instance.xrLocateHandJointsEXT(
                m_hand_tracker_right.hand_tracker,
                &hand_joints_locate_info,
                &m_hand_tracker_right.locations
            )
        );
    }
}

auto Xr_session::get_hand_tracking_joint(const XrHandEXT hand, const XrHandJointEXT joint) const -> Hand_tracking_joint
{
    const auto& tracker = (hand == XR_HAND_LEFT_EXT) ? m_hand_tracker_left : m_hand_tracker_right;
    return {
        tracker.joint_locations[joint],
        tracker.joint_velocities[joint]
    };
}

auto Xr_session::get_hand_tracking_active(const XrHandEXT hand) const -> bool
{
    const auto& tracker = (hand == XR_HAND_LEFT_EXT) ? m_hand_tracker_left : m_hand_tracker_right;
    return tracker.locations.isActive;
}

auto Xr_session::begin_frame() -> bool
{
    ERHE_PROFILE_FUNCTION

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return false;
    }

    const XrFrameBeginInfo frame_begin_info
    {
        .type = XR_TYPE_FRAME_BEGIN_INFO,
        .next = nullptr
    };

    {
        check_gl_context_in_current_in_this_thread();
        const auto result = xrBeginFrame(m_xr_session, &frame_begin_info);
        if (result == XR_SUCCESS)
        {
            return true;
        }
        if (result == XR_SESSION_LOSS_PENDING)
        {
            log_xr.error("TODO Handle XR_SESSION_LOSS_PENDING\n");
            return false;
        }
        if (result == XR_FRAME_DISCARDED)
        {
            log_xr.warn("TODO Handle XR_FRAME_DISCARDED\n");
            return true;
        }
        log_xr.error("xrBeginFrame() returned error {}\n", c_str(result));
        return false;
    }
}

auto Xr_session::wait_frame() -> XrFrameState*
{
    ERHE_PROFILE_FUNCTION

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return nullptr;
    }

    const XrFrameWaitInfo frame_wait_info{
        .type = XR_TYPE_FRAME_WAIT_INFO,
        .next = nullptr
    };

    m_xr_frame_state = XrFrameState{
        .type                   = XR_TYPE_FRAME_STATE,
        .next                   = nullptr,
        .predictedDisplayTime   = 0,
        .predictedDisplayPeriod = 0,
        .shouldRender           = XR_FALSE
    };
    {
        const auto result = xrWaitFrame(m_xr_session, &frame_wait_info, &m_xr_frame_state);
        if (result == XR_SUCCESS)
        {
            return &m_xr_frame_state;
        }
        else if (result == XR_SESSION_LOSS_PENDING)
        {
            log_xr.error("TODO Handle XR_SESSION_LOSS_PENDING\n");
            return nullptr;
        }
        else
        {
            log_xr.error("xrWaitFrame() returned error {}\n", c_str(result));
            return nullptr;
        }
    }
}

auto Xr_session::render_frame(std::function<bool(Render_view&)> render_view_callback) -> bool
{
    ERHE_PROFILE_FUNCTION

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return false;
    }

    uint32_t view_count_output{0};
    {
        ERHE_PROFILE_SCOPE("xrLocateViews");

        XrViewState view_state{
            .type           = XR_TYPE_VIEW_STATE,
            .next           = nullptr,
            .viewStateFlags = 0
        };
        uint32_t view_capacity_input{static_cast<uint32_t>(m_xr_views.size())};

        const XrViewLocateInfo view_locate_info{
            .type        = XR_TYPE_VIEW_LOCATE_INFO,
            .next        = nullptr,
            .displayTime = m_xr_frame_state.predictedDisplayTime,
            .space       = m_xr_reference_space
        };

        ERHE_XR_CHECK(
            xrLocateViews(
                m_xr_session,
                &view_locate_info,
                &view_state,
                view_capacity_input,
                &view_count_output,
                m_xr_views.data()
            )
        );
        Expects(view_count_output == view_capacity_input);
    }

    const auto& view_configuration_views = m_instance.get_xr_view_configuration_views();

    m_xr_composition_layer_projection_views.resize(view_count_output);
    m_xr_composition_layer_depth_infos     .resize(view_count_output);

    static constexpr std::string_view c_id_views{"HS views"};
    static constexpr std::string_view c_id_view{"HS view"};

    ERHE_PROFILE_GPU_SCOPE(c_id_views);
    for (uint32_t i = 0; i < view_count_output; ++i)
    {
        ERHE_PROFILE_SCOPE("view render");
        ERHE_PROFILE_GPU_SCOPE(c_id_view);

        auto& swapchain = m_view_swapchains[i];
        auto acquired_color_swapchain_image_opt = swapchain.color_swapchain.acquire();
        if (
            !acquired_color_swapchain_image_opt.has_value() ||
            !swapchain.color_swapchain.wait()
        )
        {
            return false;
        }

        auto acquired_depth_swapchain_image_opt = swapchain.depth_swapchain.acquire();
        if (
            !acquired_depth_swapchain_image_opt.has_value() ||
            !swapchain.depth_swapchain.wait()
        )
        {
            return false;
        }

        const auto& acquired_color_swapchain_image = acquired_color_swapchain_image_opt.value();
        const auto& acquired_depth_swapchain_image = acquired_depth_swapchain_image_opt.value();

        const uint32_t color_texture = acquired_color_swapchain_image.get_gl_texture();
        const uint32_t depth_texture = acquired_depth_swapchain_image.get_gl_texture();
        if (
            (color_texture == 0) ||
            (depth_texture == 0)
        )
        {
            return false;
        }

        Render_view render_view{
            .view_pose = {
                .orientation = to_glm(m_xr_views[i].pose.orientation),
                .position    = to_glm(m_xr_views[i].pose.position),
            },
            .fov_left      = m_xr_views[i].fov.angleLeft,
            .fov_right     = m_xr_views[i].fov.angleRight,
            .fov_up        = m_xr_views[i].fov.angleUp,
            .fov_down      = m_xr_views[i].fov.angleDown,
            .color_texture = color_texture,
            .depth_texture = depth_texture,
            .color_format  = m_swapchain_color_format,
            .depth_format  = m_swapchain_depth_format,
            .width         = view_configuration_views[i].recommendedImageRectWidth,
            .height        = view_configuration_views[i].recommendedImageRectHeight
        };
        {
            const auto result = render_view_callback(render_view);
            if (result == false)
            {
                return false;
            }
        }

        m_xr_composition_layer_depth_infos[i] = {
            .type     = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR,
            .next     = nullptr,
            .subImage = {
                .swapchain = swapchain.depth_swapchain.get_xr_swapchain(),
                .imageRect = {
                    .offset = { 0, 0 },
                    .extent = {
                        static_cast<int32_t>(view_configuration_views[i].recommendedImageRectWidth),
                        static_cast<int32_t>(view_configuration_views[i].recommendedImageRectHeight)
                    }
                }
            },
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
            .nearZ    = render_view.far_depth,
            .farZ     = render_view.near_depth
        };

        m_xr_composition_layer_projection_views[i] = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
            .next = &m_xr_composition_layer_depth_infos[i],
            .pose = m_xr_views[i].pose,
            .fov  = m_xr_views[i].fov,
            .subImage = {
                .swapchain = swapchain.color_swapchain.get_xr_swapchain(),
                .imageRect = {
                    .offset = { 0, 0 },
                    .extent = {
                        static_cast<int32_t>(view_configuration_views[i].recommendedImageRectWidth),
                        static_cast<int32_t>(view_configuration_views[i].recommendedImageRectHeight)
                    }
                }
            }
        };
    }

    return true;
}

auto Xr_session::end_frame() -> bool
{
    ERHE_PROFILE_FUNCTION

    if (m_xr_session == XR_NULL_HANDLE)
    {
        return false;
    }

    //XrCompositionLayerDepthTestVARJO layer_depth;
    //layer_depth.type                = XR_TYPE_COMPOSITION_LAYER_DEPTH_TEST_VARJO;
    //layer_depth.next                = nullptr;
    //layer_depth.depthTestRangeNearZ = 0.1f;
    //layer_depth.depthTestRangeFarZ  = 0.5f;

    XrCompositionLayerProjection layer{
        .type       = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .next       = nullptr,
        .layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT,
        .space      = m_xr_reference_space,
        .viewCount  = static_cast<uint32_t>(m_xr_views.size()),
        .views      = m_xr_composition_layer_projection_views.data()
    };

    XrCompositionLayerBaseHeader* layers[] = {
        reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer)
    };

    XrFrameEndInfo frame_end_info{
        .type                 = XR_TYPE_FRAME_END_INFO,
        .next                 = nullptr,
        .displayTime          = m_xr_frame_state.predictedDisplayTime,
        .environmentBlendMode = m_instance.get_xr_environment_blend_mode(),
        .layerCount           = 1,
        .layers               = layers
    };

    check_gl_context_in_current_in_this_thread();
    ERHE_XR_CHECK(xrEndFrame(m_xr_session, &frame_end_info));

    return true;
}

}
